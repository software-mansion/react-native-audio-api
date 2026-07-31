#include <audioapi/core/AudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/dsp/WsolaTimeStretcher.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr int kSampleRate = 24000;
constexpr int kQuantum = RENDER_QUANTUM_SIZE; // G.render(128)
constexpr float kPlaybackRate = 1.5f;         // WSOLA runs when pitchCorrection && rate != 1
constexpr float kOnesThreshold = 0.5f;

/// Part length in source frames. 128 is the schedule unit; may need to be longer
/// so WSOLA has enough PCM to emit (see sufficientPartFrames()).
constexpr size_t kScheduleUnitFrames = 128;

/// Default join compensation (seconds). Override without rebuilding via:
///   WSOLA_KL_SECONDS=-0.026 ./build/tests --gtest_filter='WsolaScheduleJoinLatencyTest.*'
constexpr double kL = 0.0;

double joinCompensationSeconds() {
  if (const char *env = std::getenv("WSOLA_KL_SECONDS")) {
    return std::atof(env);
  }
  return kL;
}

size_t sufficientPartFrames() {
  WsolaTimeStretcher stretcher;
  stretcher.configure(1, static_cast<float>(kSampleRate));
  const size_t minIn =
      std::max(stretcher.getRequiredInputFrames(), stretcher.getMinInputFramesToRun());
  // Enough ones after cold-start fill to emit a sustained run.
  const size_t needed = minIn + 4 * kScheduleUnitFrames;
  const size_t units = (needed + kScheduleUnitFrames - 1) / kScheduleUnitFrames;
  return units * kScheduleUnitFrames;
}

std::shared_ptr<AudioBuffer> makeOnesBuffer(size_t frames) {
  auto buffer = std::make_shared<AudioBuffer>(frames, 1, static_cast<float>(kSampleRate));
  auto *ch = buffer->getChannel(0)->begin();
  for (size_t i = 0; i < frames; ++i) {
    ch[i] = 1.0f;
  }
  return buffer;
}

struct OnesRun {
  size_t start = 0;
  size_t length = 0;
};

/// Calculates the longest continuous sequence of ones (values ≥ kOnesThreshold)
OnesRun longestOnesRun(const std::vector<float> &samples) {
  OnesRun best;
  size_t i = 0;
  while (i < samples.size()) {
    if (samples[i] < kOnesThreshold) {
      ++i;
      continue;
    }
    const size_t start = i;
    while (i < samples.size() && samples[i] >= kOnesThreshold) {
      ++i;
    }
    const size_t len = i - start;
    if (len > best.length) {
      best.start = start;
      best.length = len;
    }
  }
  return best;
}

size_t countOnesRuns(const std::vector<float> &samples) {
  size_t runs = 0;
  size_t i = 0;
  while (i < samples.size()) {
    if (samples[i] < kOnesThreshold) {
      ++i;
      continue;
    }
    ++runs;
    while (i < samples.size() && samples[i] >= kOnesThreshold) {
      ++i;
    }
  }
  return runs;
}

class TestableAudioBufferSourceNode : public AudioBufferSourceNode {
 public:
  explicit TestableAudioBufferSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioBufferSourceOptions &options)
      : AudioBufferSourceNode(context, options) {}

  using AudioBufferSourceNode::initStretch;
  using AudioBufferSourceNode::setBuffer;
};

template <typename NodeT>
NodeT *nodeOf(utils::graph::HostGraph::Node *hostNode) {
  return static_cast<NodeT *>(hostNode->handle->audioNode->asAudioNode());
}

} // namespace

/// Skeleton: two ones-buffers b1, b2 of length x, WSOLA path:
///   b1.start(0)
///   b2.start(x / (sr * playbackRate) + L)
///   repeatedly G.render(128)
/// Pass when output is: leading zeros (WSOLA latency), then ONE contiguous ones
/// run (no internal gap). A gap ⇒ L is wrong.
///
/// kL is a stub (0). An agent should derive the formula for L that makes this pass.
/// No search loop here — single fixed L only.
TEST(WsolaScheduleJoinLatencyTest, ContiguousOnesAfterJoin) {
  const double L = joinCompensationSeconds();
  const size_t x = sufficientPartFrames();
  // Ideal wall-clock ones from two abutted parts (OLA soft edges ⇒ allow < 100%).
  const size_t idealOnes = static_cast<size_t>(
      std::lround(2.0 * static_cast<double>(x) / static_cast<double>(kPlaybackRate)));
  const size_t minOnes = (idealOnes * 90) / 100;
  const size_t captureFrames = idealOnes + static_cast<size_t>(kSampleRate / 2); // 0.5s margin

  auto eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
  const size_t safetyMarginSize = 2;
  auto context = std::make_shared<OfflineAudioContext>(
      1, static_cast<int>(captureFrames * safetyMarginSize), kSampleRate, eventRegistry);
  auto destination = std::make_unique<AudioDestinationNode>(context);
  context->initialize(destination.get());
  auto *destinationHostNode = context->getGraph()->addNode(std::move(destination));

  BaseAudioBufferSourceOptions baseOptions;
  baseOptions.pitchCorrection = true;
  baseOptions.playbackRate = kPlaybackRate;
  baseOptions.detune = 0.0f;
  AudioBufferSourceOptions options(baseOptions);

  auto makeSource = [&]() {
    auto node = std::make_unique<TestableAudioBufferSourceNode>(context, options);
    auto playbackRateBuffer = std::make_shared<DSPAudioBuffer>(
        WsolaTimeStretcher::scratchBufferFrames(static_cast<float>(kSampleRate)),
        1,
        static_cast<float>(kSampleRate));
    node->initStretch(1, static_cast<float>(kSampleRate), playbackRateBuffer);
    auto outBuf = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
    node->setBuffer(makeOnesBuffer(x), outBuf);
    auto *hostNode = context->getGraph()->addNode(std::move(node));
    EXPECT_TRUE(context->getGraph()->addEdge(hostNode, destinationHostNode).is_ok());
    return nodeOf<TestableAudioBufferSourceNode>(hostNode);
  };

  auto *b1 = makeSource();
  auto *b2 = makeSource();

  const double contentDur = static_cast<double>(x) /
      (static_cast<double>(kSampleRate) * static_cast<double>(kPlaybackRate));
  b1->start(0.0, 0.0);
  b2->start(contentDur + L, 0.0);

  std::vector<float> output;
  output.reserve(captureFrames);
  auto quantum = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
  while (output.size() < captureFrames) {
    quantum->zero();
    context->processGraph(quantum.get(), kQuantum); // G.render(128)
    const float *ch = quantum->getChannel(0)->begin();
    for (int i = 0; i < kQuantum; ++i) {
      output.push_back(ch[i]);
    }
  }

  const OnesRun run = longestOnesRun(output);
  const size_t runs = countOnesRuns(output);

  EXPECT_EQ(runs, 1u) << "Gap in ones ⇒ L is wrong. L=" << L << "s onesStart=" << run.start
                      << " onesLen=" << run.length;
  EXPECT_GE(run.start, 1u) << "Expected leading zeros from WSOLA latency before ones";
  EXPECT_GE(run.length, minOnes) << "Contiguous ones too short (need ≥90% of 2*x/rate). L=" << L
                                 << "s onesLen=" << run.length << " min=" << minOnes
                                 << " ideal=" << idealOnes;
}

// NOLINTEND
