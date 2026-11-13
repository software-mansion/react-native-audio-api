#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {
void appendU16LE(std::vector<uint8_t> &bytes, uint16_t value) {
  bytes.push_back(static_cast<uint8_t>(value & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void appendU32LE(std::vector<uint8_t> &bytes, uint32_t value) {
  bytes.push_back(static_cast<uint8_t>(value & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
  bytes.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

/// A mono 16-bit WAV holding `frameCount` frames of a constant half-scale sample.
std::vector<uint8_t> makeTestWavPcm16Mono(size_t frameCount) {
  constexpr uint16_t channels = 1;
  constexpr uint32_t sampleRate = 44100;
  constexpr uint16_t bitsPerSample = 16;
  constexpr uint16_t bytesPerSample = bitsPerSample / 8;
  constexpr uint16_t blockAlign = channels * bytesPerSample;
  constexpr uint32_t byteRate = sampleRate * blockAlign;

  const std::vector<int16_t> samples(frameCount, 16384);
  const uint32_t dataChunkSize = static_cast<uint32_t>(samples.size() * sizeof(int16_t));
  const uint32_t riffChunkSize = 36 + dataChunkSize;

  std::vector<uint8_t> wav;
  wav.reserve(44 + dataChunkSize);

  wav.insert(wav.end(), {'R', 'I', 'F', 'F'});
  appendU32LE(wav, riffChunkSize);
  wav.insert(wav.end(), {'W', 'A', 'V', 'E'});

  wav.insert(wav.end(), {'f', 'm', 't', ' '});
  appendU32LE(wav, 16);
  appendU16LE(wav, 1);
  appendU16LE(wav, channels);
  appendU32LE(wav, sampleRate);
  appendU32LE(wav, byteRate);
  appendU16LE(wav, blockAlign);
  appendU16LE(wav, bitsPerSample);

  wav.insert(wav.end(), {'d', 'a', 't', 'a'});
  appendU32LE(wav, dataChunkSize);

  const auto *sampleBytes = reinterpret_cast<const uint8_t *>(samples.data());
  wav.insert(wav.end(), sampleBytes, sampleBytes + dataChunkSize);

  return wav;
}

/// Returns the concrete node behind a HostVertex created via Graph::addNode
/// (the unique_ptr payload lives inside the NodeHandle).
template <typename NodeT>
NodeT *nodeOf(utils::graph::HostGraph::HostVertex *hostNode) {
  return static_cast<NodeT *>(hostNode->handle->audioNode->asAudioNode());
}
} // namespace

class MediaElementAudioSourceNodeTest : public ::testing::Test {
 protected:
  using HostVertex = utils::graph::HostGraph::HostVertex;

  static constexpr int SAMPLE_RATE = 44100;
  static constexpr size_t SHORT_FILE_FRAMES = 4;
  static constexpr size_t HALF_SECOND_FILE_FRAMES = SAMPLE_RATE / 2;

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  utils::graph::Graph *graph = nullptr;
  HostVertex *destinationHostNode = nullptr;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * SAMPLE_RATE, SAMPLE_RATE, eventRegistry);
    auto destination = std::make_unique<AudioDestinationNode>(context);
    context->initialize(destination.get());
    graph = context->getGraph().get();
    destinationHostNode = graph->addNode(std::move(destination));
  }

  HostVertex *addFileSource(size_t frameCount) {
    AudioFileSourceOptions options;
    options.data = makeTestWavPcm16Mono(frameCount);
    return graph->addNode(std::make_unique<AudioFileSourceNode>(context, options));
  }

  /// A media element node wired to the destination, as an application would do.
  HostVertex *addMediaElementSourceConnectedToDestination() {
    auto *mediaHostNode = graph->addNode(std::make_unique<MediaElementAudioSourceNode>(context));
    EXPECT_TRUE(graph->addEdge(mediaHostNode, destinationHostNode).is_ok());
    return mediaHostNode;
  }

  /// Renders one quantum through the real graph and returns the loudest
  /// sample that reached the destination.
  float renderQuantumPeak() {
    auto out =
        std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, 2, static_cast<float>(SAMPLE_RATE));
    context->processGraph(out.get(), RENDER_QUANTUM_SIZE);
    float peak = 0.0f;
    for (size_t frame = 0; frame < RENDER_QUANTUM_SIZE; ++frame) {
      peak = std::max(peak, std::abs((*out->getChannel(0))[frame]));
    }
    return peak;
  }

  /// Renders until `condition` holds. The decoder fills its queue from another
  /// thread, so audio and the end of the file arrive after an unknown number
  /// of quanta.
  template <typename Condition>
  bool renderUntil(Condition condition) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
      const float peak = renderQuantumPeak();
      if (condition(peak)) {
        return true;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return false;
  }

  static bool isAudible(float peak) {
    return peak > 0.1f;
  }

  /// What the graph sees when JS drops a node: its edges go, then the node.
  void releaseFromJs(HostVertex *hostNode) {
    ASSERT_TRUE(graph->removeAllEdges(hostNode).is_ok());
    ASSERT_TRUE(graph->removeNode(hostNode).is_ok());
  }

  /// Whether the graph lets go of the node behind `handle`. Compaction runs on
  /// the render path and the handles captured by graph events are released by
  /// the disposer thread, so this takes an unknown number of quanta.
  bool isFreedAfterRendering(const std::weak_ptr<utils::graph::NodeHandle> &handle) {
    return renderUntil([&](float) {
      graph->collectDisposedNodes();
      return handle.expired();
    });
  }
};

TEST_F(MediaElementAudioSourceNodeTest, ForwardsItsFileSourceToTheDestination) {
  auto *fileSourceHostNode = addFileSource(HALF_SECOND_FILE_FRAMES);
  auto *mediaHostNode = addMediaElementSourceConnectedToDestination();
  ASSERT_TRUE(graph->addEdge(fileSourceHostNode, mediaHostNode).is_ok());

  nodeOf<AudioFileSourceNode>(fileSourceHostNode)->start(0);

  EXPECT_TRUE(renderUntil(isAudible));
}

/// <Audio> replaces its file source after every playthrough. The application's
/// media element node has to survive that and carry the replacement.
TEST_F(MediaElementAudioSourceNodeTest, OutlivesItsFileSourceAndForwardsTheReplacement) {
  auto *mediaHostNode = addMediaElementSourceConnectedToDestination();

  auto *finishedHostNode = addFileSource(SHORT_FILE_FRAMES);
  ASSERT_TRUE(graph->addEdge(finishedHostNode, mediaHostNode).is_ok());
  auto *finishedFileSource = nodeOf<AudioFileSourceNode>(finishedHostNode);
  std::weak_ptr<utils::graph::NodeHandle> finishedHandle = finishedHostNode->handle;
  finishedFileSource->start(0);
  ASSERT_TRUE(renderUntil([&](float) { return finishedFileSource->isFinished(); }));

  releaseFromJs(finishedHostNode);
  ASSERT_TRUE(isFreedAfterRendering(finishedHandle));
  EXPECT_FALSE(isAudible(renderQuantumPeak()));

  auto *replacementHostNode = addFileSource(HALF_SECOND_FILE_FRAMES);
  ASSERT_TRUE(graph->addEdge(replacementHostNode, mediaHostNode).is_ok());
  nodeOf<AudioFileSourceNode>(replacementHostNode)->start(context->getCurrentTime());

  EXPECT_TRUE(renderUntil(isAudible));
}

TEST_F(MediaElementAudioSourceNodeTest, IsFreedOnceReleasedAfterItsFileSourceFinished) {
  auto *mediaHostNode = addMediaElementSourceConnectedToDestination();
  auto *fileSourceHostNode = addFileSource(SHORT_FILE_FRAMES);
  ASSERT_TRUE(graph->addEdge(fileSourceHostNode, mediaHostNode).is_ok());
  auto *fileSource = nodeOf<AudioFileSourceNode>(fileSourceHostNode);
  std::weak_ptr<utils::graph::NodeHandle> mediaHandle = mediaHostNode->handle;
  std::weak_ptr<utils::graph::NodeHandle> fileSourceHandle = fileSourceHostNode->handle;
  fileSource->start(0);
  ASSERT_TRUE(renderUntil([&](float) { return fileSource->isFinished(); }));

  releaseFromJs(fileSourceHostNode);
  releaseFromJs(mediaHostNode);

  EXPECT_TRUE(isFreedAfterRendering(fileSourceHandle));
  EXPECT_TRUE(isFreedAfterRendering(mediaHandle));
}

/// A playing <Audio> keeps sounding through a media element node the
/// application no longer references, like any playing source keeps its chain.
TEST_F(MediaElementAudioSourceNodeTest, StaysWhileItsFileSourceIsPlaying) {
  auto *mediaHostNode = addMediaElementSourceConnectedToDestination();
  auto *fileSourceHostNode = addFileSource(HALF_SECOND_FILE_FRAMES);
  ASSERT_TRUE(graph->addEdge(fileSourceHostNode, mediaHostNode).is_ok());
  std::weak_ptr<utils::graph::NodeHandle> mediaHandle = mediaHostNode->handle;
  nodeOf<AudioFileSourceNode>(fileSourceHostNode)->start(0);
  ASSERT_TRUE(renderUntil(isAudible));

  ASSERT_TRUE(graph->removeNode(mediaHostNode).is_ok());
  for (int quantum = 0; quantum < 8; ++quantum) {
    EXPECT_TRUE(isAudible(renderQuantumPeak()));
    graph->collectDisposedNodes();
  }

  EXPECT_FALSE(mediaHandle.expired());
}

/// A stop time that comes while the source is paused ends it then; the pause
/// must not hide the stop from the render path.
TEST_F(MediaElementAudioSourceNodeTest, PausedFileSourceFinishesAtItsStopTime) {
  auto *mediaHostNode = addMediaElementSourceConnectedToDestination();
  auto *fileSourceHostNode = addFileSource(SHORT_FILE_FRAMES);
  ASSERT_TRUE(graph->addEdge(fileSourceHostNode, mediaHostNode).is_ok());
  auto *fileSource = nodeOf<AudioFileSourceNode>(fileSourceHostNode);

  fileSource->start(0);
  fileSource->pause();
  // Mid-quantum, so frame rounding cannot move the stop across a quantum boundary.
  fileSource->stop(2.5 * RENDER_QUANTUM_SIZE / SAMPLE_RATE);

  renderQuantumPeak();
  renderQuantumPeak();
  EXPECT_FALSE(fileSource->hasEnded());

  renderQuantumPeak(); // the quantum holding the stop time
  EXPECT_TRUE(fileSource->hasEnded());
  EXPECT_TRUE(fileSource->isFinished());
}

// NOLINTEND
