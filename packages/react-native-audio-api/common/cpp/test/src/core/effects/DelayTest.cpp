#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/effects/delay/DelayReader.h>
#include <audioapi/core/effects/delay/DelayWriter.h>
#include <audioapi/core/utils/graph/BridgeNode.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

class DelayTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }
};

class TestableDelayWriter : public DelayWriter {
 public:
  explicit TestableDelayWriter(
      std::shared_ptr<BaseAudioContext> context,
      const DelayOptions &options,
      std::shared_ptr<DelayLine> delayLine)
      : DelayWriter(context, options, delayLine) {}

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    audioBuffer_ = input;
  }
};

class TestableDelayReader : public DelayReader {
 public:
  explicit TestableDelayReader(
      std::shared_ptr<BaseAudioContext> context,
      const DelayOptions &options,
      std::shared_ptr<DelayLine> delayLine)
      : DelayReader(context, options, delayLine) {}

  auto getOutputBuffer() {
    return audioBuffer_;
  }
};

class TestableDelayNode : public DelayNode {
 public:
  explicit TestableDelayNode(std::shared_ptr<BaseAudioContext> context, const DelayOptions &options)
      : DelayNode(context, options),
        testableDelayReader_(context, options, delayLine_),
        testableDelayWriter_(context, options, delayLine_) {}

  void setDelayTimeParam(float value) {
    getDelayTimeParam()->setValue(value);
  }

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    testableDelayWriter_.setInputBuffer(input);
  }

  auto getOutputBuffer() {
    return testableDelayReader_.getOutputBuffer();
  }

  void processNode(int framesToProcess) override {
    testableDelayWriter_.processNode(framesToProcess);
    testableDelayReader_.processNode(framesToProcess);
  }

 private:
  TestableDelayWriter testableDelayWriter_;
  TestableDelayReader testableDelayReader_;
};

TEST_F(DelayTest, DelayCanBeCreated) {
  auto delay = std::make_shared<DelayNode>(context, DelayOptions());
  ASSERT_NE(delay, nullptr);
}

TEST_F(DelayTest, DelayWithZeroDelayOutputsInputSignal) {
  static constexpr float DELAY_TIME = 0.0f;
  static constexpr int FRAMES_TO_PROCESS = 128;
  auto options = DelayOptions();
  options.maxDelayTime = 1.0f;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInputBuffer(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getOutputBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], static_cast<float>(i + 1));
  }
}

TEST_F(DelayTest, DelayAppliesTimeShiftCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 128;
  float DELAY_TIME = (FRAMES_TO_PROCESS / context->getSampleRate()) * 0.5;
  auto options = DelayOptions();
  options.maxDelayTime = 1.0f;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInputBuffer(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getOutputBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (i < FRAMES_TO_PROCESS / 2) { // First 64 samples should be zero due to delay
      EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i], 0.0f);
    } else {
      EXPECT_FLOAT_EQ(
          (*resultBuffer->getChannel(0))[i],
          static_cast<float>(
              i + 1 - FRAMES_TO_PROCESS / 2.0)); // Last 64 samples should be 1st part of buffer
    }
  }
}

TEST_F(DelayTest, DelayHandlesTailCorrectly) {
  static constexpr int FRAMES_TO_PROCESS = 128;
  float DELAY_TIME = (FRAMES_TO_PROCESS / context->getSampleRate()) * 0.5;
  auto options = DelayOptions();
  options.maxDelayTime = (FRAMES_TO_PROCESS / context->getSampleRate()) * 3;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInputBuffer(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  // Second call uses the result of the first call as input
  delayNode.getOutputBuffer()->zero();
  delayNode.processNode(FRAMES_TO_PROCESS);
  auto resultBuffer = delayNode.getOutputBuffer();
  for (size_t i = 0; i < FRAMES_TO_PROCESS; ++i) {
    if (i < FRAMES_TO_PROCESS / 2) { // First samples should be 2nd part of buffer
      EXPECT_FLOAT_EQ(
          (*resultBuffer->getChannel(0))[i], static_cast<float>(i + 1 + FRAMES_TO_PROCESS / 2.0));
    } else {
      EXPECT_FLOAT_EQ((*resultBuffer->getChannel(0))[i],
                      0.0f); // Last samples should be zero
    }
  }
}

namespace {

/// Emits a single 1.0 sample at the very first rendered frame, silence after.
class ClickNode : public AudioNode {
 public:
  explicit ClickNode(const std::shared_ptr<BaseAudioContext> &context)
      : AudioNode(context, clickOptions()) {}

 protected:
  void processNode(int /*framesToProcess*/) override {
    audioBuffer_->zero();
    if (!fired_) {
      (*audioBuffer_->getChannel(0))[0] = 1.0f;
      fired_ = true;
    }
  }

 private:
  bool fired_ = false;

  static AudioNodeOptions clickOptions() {
    AudioNodeOptions options;
    options.channelCount = 1;
    options.numberOfInputs = 0;
    return options;
  }
};

/// Emits a constant value on every frame (stands in for ConstantSourceNode / an LFO).
class ConstantSignalNode : public AudioNode {
 public:
  ConstantSignalNode(const std::shared_ptr<BaseAudioContext> &context, float value)
      : AudioNode(context, signalOptions()), value_(value) {}

 protected:
  void processNode(int framesToProcess) override {
    auto channel = audioBuffer_->getChannel(0)->span();
    std::fill(channel.begin(), channel.begin() + framesToProcess, value_);
  }

 private:
  float value_;

  static AudioNodeOptions signalOptions() {
    AudioNodeOptions options;
    options.channelCount = 1;
    options.numberOfInputs = 0;
    return options;
  }
};

} // namespace

/// Renders through the real graph with the reader / writer / delayTime-bridge
/// wiring that DelayNodeHostObject builds, so graph ordering and param
/// routing are exercised (the processNode-level tests above call the writer
/// before the reader by hand and cannot see either).
class DelayGraphTest : public ::testing::Test {
 protected:
  static constexpr int kSampleRate = 16000;
  static constexpr int kQuantum = 128;
  static constexpr float kMaxDelayTime = 1.0f;

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<utils::graph::Graph> graph;
  utils::graph::HostGraph::HostVertex *destinationVertex = nullptr;
  utils::graph::HostGraph::HostVertex *writerVertex = nullptr;
  std::unique_ptr<DelayNode> delayNode;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(1, 2 * kSampleRate, kSampleRate, eventRegistry);
    graph = context->getGraph();
    auto destination = std::make_unique<AudioDestinationNode>(context);
    context->initialize(destination.get());
    destinationVertex = graph->addNode(std::move(destination));
  }

  utils::graph::HostGraph::HostVertex *readerVertex = nullptr;

  /// click -> writer ~ring~ reader -> destination, with reader linked to writer.
  void buildDelay(float delayTime) {
    DelayOptions options;
    options.channelCount = 1;
    options.maxDelayTime = kMaxDelayTime;
    options.delayTime = delayTime;
    delayNode = std::make_unique<DelayNode>(context, options);

    // Same creation order as DelayNodeHostObject: reader first, then writer.
    readerVertex = graph->addNode(std::move(delayNode->delayReader_));
    writerVertex = graph->addNode(std::move(delayNode->delayWriter_));
    graph->linkNodes(readerVertex, writerVertex);

    auto *clickVertex = graph->addNode(std::make_unique<ClickNode>(context));
    ASSERT_TRUE(graph->addEdge(clickVertex, writerVertex).is_ok());
    ASSERT_TRUE(graph->addEdge(readerVertex, destinationVertex).is_ok());
  }

  /// signal -> bridge -> writer, the edge AudioParamHostObject adds for `signal.connect(delayTime)`.
  void connectSignalToDelayTime(float value) {
    auto *signalVertex = graph->addNode(std::make_unique<ConstantSignalNode>(context, value));
    auto *bridgeVertex = graph->addNode(
        std::make_unique<utils::graph::BridgeNode>(delayNode->getDelayTimeParam().get()));
    ASSERT_TRUE(graph->addEdge(signalVertex, bridgeVertex).is_ok());
    ASSERT_TRUE(graph->addEdge(bridgeVertex, writerVertex).is_ok());
  }

  /// @return the frame at which the click reaches the destination, or -1.
  int renderAndFindClickFrame(int maxFrames) {
    auto out = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
    for (int quantumStart = 0; quantumStart < maxFrames; quantumStart += kQuantum) {
      out->zero();
      context->processGraph(out.get(), kQuantum);
      auto samples = out->getChannel(0)->span();
      for (int i = 0; i < kQuantum; ++i) {
        if (samples[i] > 0.5f) {
          return quantumStart + i;
        }
      }
    }
    return -1;
  }
};

TEST_F(DelayGraphTest, ZeroDelayTimeOutputsClickImmediately) {
  buildDelay(0.0f);
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), 0);
}

TEST_F(DelayGraphTest, SubQuantumDelayIsSampleAccurate) {
  buildDelay(37.0f / kSampleRate);
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), 37);
}

TEST_F(DelayGraphTest, DelayTimeValueDelaysClick) {
  buildDelay(0.25f);
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), kSampleRate / 4);
}

TEST_F(DelayGraphTest, SignalConnectedToDelayTimeAddsToItsValue) {
  buildDelay(0.0f);
  connectSignalToDelayTime(0.25f);
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), kSampleRate / 4);
}

TEST_F(DelayGraphTest, SignalAndValueSumOnDelayTime) {
  buildDelay(0.25f);
  connectSignalToDelayTime(0.25f);
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), kSampleRate / 2);
}

TEST_F(DelayGraphTest, FeedbackLoopClampsDelayToOneQuantum) {
  // Spec: a DelayNode inside a cycle has its delay clamped to at least one
  // render quantum, since the reader is processed before the writer there.
  buildDelay(0.0f);
  ASSERT_TRUE(graph->addEdge(readerVertex, writerVertex).is_ok());
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), kQuantum);
}

TEST_F(DelayGraphTest, MaxDelayTimeDoesNotWrapOntoCurrentReadWindow) {
  buildDelay(kMaxDelayTime);
  EXPECT_EQ(
      renderAndFindClickFrame(2 * kSampleRate), static_cast<int>(kMaxDelayTime * kSampleRate));
}

// NOLINTEND
