#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/core/utils/graph/BridgeNode.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>
#include <vector>

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

class TestableDelayNode : public DelayNode {
 public:
  explicit TestableDelayNode(std::shared_ptr<BaseAudioContext> context, const DelayOptions &options)
      : DelayNode(context, options) {}

  void setDelayTimeParam(float value) {
    getDelayTimeParam()->setValue(value);
  }

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    audioBuffer_ = input;
  }

  auto getOutputBuffer() {
    return audioBuffer_;
  }

  using DelayNode::processNode;
};

TEST_F(DelayTest, DelayCanBeCreated) {
  auto delay = std::make_shared<DelayNode>(context, DelayOptions());
  ASSERT_NE(delay, nullptr);
}

TEST_F(DelayTest, DelayWithZeroDelayOutputsInputSignal) {
  static constexpr float DELAY_TIME = 0.0f;
  static constexpr int FRAMES_TO_PROCESS = 128;
  auto options = DelayOptions();
  options.channelCount = 1; // matches the mono buffer setInputBuffer installs
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
  options.channelCount = 1; // matches the mono buffer setInputBuffer installs
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
  options.channelCount = 1; // matches the mono buffer setInputBuffer installs
  options.maxDelayTime = (FRAMES_TO_PROCESS / context->getSampleRate()) * 3;
  auto delayNode = TestableDelayNode(context, options);
  delayNode.setDelayTimeParam(DELAY_TIME);

  auto buffer = std::make_shared<audioapi::DSPAudioBuffer>(FRAMES_TO_PROCESS, 1, sampleRate);
  for (size_t i = 0; i < buffer->getSize(); ++i) {
    (*buffer->getChannel(0))[i] = i + 1;
  }

  delayNode.setInputBuffer(buffer);
  delayNode.processNode(FRAMES_TO_PROCESS);
  // The second quantum has silent input
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

/// Emits a single 1.0 sample at `clickFrame` of the first quantum on
/// `clickChannel` of `numberOfChannels`, silence after.
class ClickNode : public AudioNode {
 public:
  explicit ClickNode(
      const std::shared_ptr<BaseAudioContext> &context,
      int clickFrame = 0,
      int numberOfChannels = 1,
      int clickChannel = 0)
      : AudioNode(context, clickOptions(numberOfChannels)),
        clickFrame_(clickFrame),
        clickChannel_(clickChannel) {}

 protected:
  void processNode(int /*framesToProcess*/) override {
    audioBuffer_->zero();
    if (!fired_) {
      (*audioBuffer_->getChannel(clickChannel_))[clickFrame_] = 1.0f;
      fired_ = true;
    }
  }

 private:
  int clickFrame_;
  int clickChannel_;
  bool fired_ = false;

  static AudioNodeOptions clickOptions(int numberOfChannels) {
    AudioNodeOptions options;
    options.channelCount = numberOfChannels;
    options.numberOfInputs = 0;
    return options;
  }
};

/// Emits 0 for the first half of every quantum and `value` for the second, a
/// signal whose first sample says nothing about the rest of the quantum.
class HalfQuantumStepNode : public AudioNode {
 public:
  HalfQuantumStepNode(const std::shared_ptr<BaseAudioContext> &context, float value)
      : AudioNode(context, signalOptions()), value_(value) {}

 protected:
  void processNode(int framesToProcess) override {
    auto channel = audioBuffer_->getChannel(0)->span();
    const int half = framesToProcess / 2;
    std::fill(channel.begin(), channel.begin() + half, 0.0f);
    std::fill(channel.begin() + half, channel.begin() + framesToProcess, value_);
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

/// Renders through the real graph with the delayTime-bridge wiring that
/// DelayNodeHostObject builds, so graph ordering, feedback cycles and param
/// routing are exercised (the processNode-level tests above cannot see them).
class DelayGraphTest : public ::testing::Test {
 protected:
  static constexpr int kSampleRate = 16000;
  static constexpr int kQuantum = 128;
  static constexpr float kMaxDelayTime = 1.0f;

  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<utils::graph::Graph> graph;
  utils::graph::HostGraph::HostVertex *destinationVertex = nullptr;
  utils::graph::HostGraph::HostVertex *delayVertex = nullptr;
  DelayNode *delayNode = nullptr;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(1, 2 * kSampleRate, kSampleRate, eventRegistry);
    graph = context->getGraph();
    auto destination = std::make_unique<AudioDestinationNode>(context);
    context->initialize(destination.get());
    destinationVertex = graph->addNode(std::move(destination));
  }

  utils::graph::HostGraph::HostVertex *addDelay(float delayTime) {
    DelayOptions options;
    options.channelCount = 1;
    options.maxDelayTime = kMaxDelayTime;
    options.delayTime = delayTime;
    auto node = std::make_unique<DelayNode>(context, options);
    delayNode = node.get();
    return graph->addNode(std::move(node));
  }

  /// click -> delay -> destination
  void buildDelay(float delayTime) {
    delayVertex = addDelay(delayTime);
    auto *clickVertex = graph->addNode(std::make_unique<ClickNode>(context));
    ASSERT_TRUE(graph->addEdge(clickVertex, delayVertex).is_ok());
    ASSERT_TRUE(graph->addEdge(delayVertex, destinationVertex).is_ok());
  }

  /// signal -> bridge -> delay, the edge AudioParamHostObject adds for `signal.connect(delayTime)`.
  void connectToDelayTime(std::unique_ptr<AudioNode> signal) {
    auto *signalVertex = graph->addNode(std::move(signal));
    auto *bridgeVertex = graph->addNode(
        std::make_unique<utils::graph::BridgeNode>(delayNode->getDelayTimeParam().get()));
    ASSERT_TRUE(graph->addEdge(signalVertex, bridgeVertex).is_ok());
    ASSERT_TRUE(graph->addEdge(bridgeVertex, delayVertex).is_ok());
  }

  void connectSignalToDelayTime(float value) {
    connectToDelayTime(std::make_unique<ConstantSignalNode>(context, value));
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
  // render quantum.
  buildDelay(0.0f);
  ASSERT_TRUE(graph->addEdge(delayVertex, delayVertex).is_ok());
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), kQuantum);
}

TEST_F(DelayGraphTest, LongerDelayInFeedbackLoopKeepsItsDelayTime) {
  buildDelay(0.25f);
  ASSERT_TRUE(graph->addEdge(delayVertex, delayVertex).is_ok());
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), kSampleRate / 4);
}

TEST_F(DelayGraphTest, BreakingTheFeedbackLoopRestoresZeroDelay) {
  buildDelay(0.0f);
  ASSERT_TRUE(graph->addEdge(delayVertex, delayVertex).is_ok());
  ASSERT_TRUE(graph->removeEdge(delayVertex, delayVertex).is_ok());
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), 0);
}

TEST_F(DelayGraphTest, DelayFeedingAFeedbackLoopKeepsItsZeroDelay) {
  // The zero delay only feeds the loop and is not part of it, so it must not
  // be clamped.
  auto *loopDelayVertex = addDelay(0.25f);
  buildDelay(0.0f);
  auto *mixVertex = graph->addNode(std::make_unique<GainNode>(context, GainOptions{}));
  ASSERT_TRUE(graph->addEdge(delayVertex, mixVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(loopDelayVertex, mixVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(mixVertex, loopDelayVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(mixVertex, destinationVertex).is_ok());

  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), 0);
}

TEST_F(DelayGraphTest, TwoDelaysFeedingEachOtherEachAddOneQuantum) {
  // click -> first -> second -> destination, second -> first. Both delays lie
  // on the cycle, so the click crosses two clamped delays.
  auto *secondVertex = addDelay(0.0f);
  auto *firstVertex = addDelay(0.0f);
  auto *clickVertex = graph->addNode(std::make_unique<ClickNode>(context));
  ASSERT_TRUE(graph->addEdge(clickVertex, firstVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(firstVertex, secondVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(secondVertex, firstVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(secondVertex, destinationVertex).is_ok());

  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), 2 * kQuantum);
}

TEST_F(DelayGraphTest, CycleWithoutADelayIsRejected) {
  auto *first = graph->addNode(std::make_unique<GainNode>(context, GainOptions{}));
  auto *second = graph->addNode(std::make_unique<GainNode>(context, GainOptions{}));
  ASSERT_TRUE(graph->addEdge(first, second).is_ok());
  EXPECT_TRUE(graph->addEdge(second, first).is_err());
}

TEST_F(DelayGraphTest, MaxDelayTimeDoesNotWrapOntoCurrentReadWindow) {
  buildDelay(kMaxDelayTime);
  EXPECT_EQ(
      renderAndFindClickFrame(2 * kSampleRate), static_cast<int>(kMaxDelayTime * kSampleRate));
}

TEST_F(DelayGraphTest, DelayTimeIsAppliedWhenTheFrameIsRead) {
  // output(t) = input(t - delayTime(t)): the click written at frame 0 under a
  // 256-frame delay comes out at frame 128 once the delay drops to 128 there.
  buildDelay(0.0f);
  auto delayTime = delayNode->getDelayTimeParam();
  delayTime->setValueAtTime(256.0f / kSampleRate, 0.0);
  delayTime->setValueAtTime(128.0f / kSampleRate, 128.0 / kSampleRate);
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), 128);
}

TEST_F(DelayGraphTest, ConnectedDelayTimeIsAudioRate) {
  // The signal is 0 at the start of the quantum, so a k-rate read would keep
  // the click at frame 70; the a-rate value at frame 90 is 20 frames.
  delayVertex = addDelay(0.0f);
  auto *clickVertex = graph->addNode(std::make_unique<ClickNode>(context, 70));
  ASSERT_TRUE(graph->addEdge(clickVertex, delayVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(delayVertex, destinationVertex).is_ok());
  connectToDelayTime(std::make_unique<HalfQuantumStepNode>(context, 20.0f / kSampleRate));
  EXPECT_EQ(renderAndFindClickFrame(2 * kSampleRate), 90);
}

TEST_F(DelayGraphTest, DelayKeepsEveryNegotiatedChannel) {
  // Two-channel DelayNode in MAX mode mixing a four-channel input discretely:
  // the ring must grow to four channels, or channel 3 is dropped.
  DelayOptions options;
  options.channelCount = 2;
  options.channelInterpretation = ChannelInterpretation::DISCRETE;
  options.maxDelayTime = kMaxDelayTime;
  auto node = std::make_unique<DelayNode>(context, options);
  delayNode = node.get();
  delayVertex = graph->addNode(std::move(node));
  auto *clickVertex = graph->addNode(std::make_unique<ClickNode>(context, 5, 4, 3));
  ASSERT_TRUE(graph->addEdge(clickVertex, delayVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(delayVertex, destinationVertex).is_ok());

  auto out = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
  context->processGraph(out.get(), kQuantum);
  auto delayOutput = delayNode->getOutputBuffer();
  ASSERT_EQ(delayOutput->getNumberOfChannels(), 4u);
  EXPECT_EQ((*delayOutput->getChannel(3))[5], 1.0f);
}

TEST_F(DelayGraphTest, ChannelCountPropagatesAroundAFeedbackLoop) {
  // a -> b -> c -> delay -> c, a -> delay, then stereo -> a. With b fixed to
  // mono, c only learns it is stereo from the delay, which the negotiation
  // reaches after c.
  GainOptions monoOptions;
  monoOptions.channelCount = 1;
  GainOptions explicitMonoOptions = monoOptions;
  explicitMonoOptions.channelCountMode = ChannelCountMode::EXPLICIT;
  auto *a = graph->addNode(std::make_unique<GainNode>(context, monoOptions));
  auto *b = graph->addNode(std::make_unique<GainNode>(context, explicitMonoOptions));
  auto c = std::make_unique<GainNode>(context, monoOptions);
  auto *cNode = c.get();
  auto *cVertex = graph->addNode(std::move(c));
  auto *loopDelay = addDelay(0.0f);
  ASSERT_TRUE(graph->addEdge(a, b).is_ok());
  ASSERT_TRUE(graph->addEdge(b, cVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(cVertex, loopDelay).is_ok());
  ASSERT_TRUE(graph->addEdge(loopDelay, cVertex).is_ok());
  ASSERT_TRUE(graph->addEdge(a, loopDelay).is_ok());
  ASSERT_TRUE(graph->addEdge(cVertex, destinationVertex).is_ok());

  auto *stereo = graph->addNode(std::make_unique<ClickNode>(context, 0, 2));
  ASSERT_TRUE(graph->addEdge(stereo, a).is_ok());

  auto out = std::make_shared<DSPAudioBuffer>(kQuantum, 1, static_cast<float>(kSampleRate));
  context->processGraph(out.get(), kQuantum);
  EXPECT_EQ(cNode->getOutputBuffer()->getNumberOfChannels(), 2u);
}

// NOLINTEND
