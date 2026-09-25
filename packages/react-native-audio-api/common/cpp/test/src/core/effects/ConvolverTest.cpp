#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/Convolver.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/ThreadPool.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <algorithm>
#include <memory>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

// Spec channel configurations under test:
// https://webaudio.github.io/web-audio-api/#Convolution-channel-configurations
//
// The node processes in place in a buffer negotiation sizes at the output
// width (see ConvolverNode::negotiateBufferChannelCount). A mono input into a
// stereo buffer arrives up-mixed by the graph mixer, so `inputWithChannels`
// fills every extra buffer channel with the last real input channel.

class ConvolverTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  std::shared_ptr<AudioDestinationNode> destination;
  static constexpr int sampleRate = 44100;
  static constexpr int FRAMES = RENDER_QUANTUM_SIZE;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
    destination = std::make_shared<AudioDestinationNode>(context);
    context->initialize(destination.get());
  }

  /// Impulse response whose channel `c` is a single unit tap at `taps[c]`.
  std::shared_ptr<AudioBuffer> impulseResponseWithTaps(const std::vector<size_t> &taps) {
    constexpr size_t kLength = 4;
    auto ir = std::make_shared<AudioBuffer>(kLength, static_cast<int>(taps.size()), sampleRate);
    ir->zero();
    for (size_t c = 0; c < taps.size(); ++c) {
      (*ir->getChannel(c))[taps[c]] = 1.0f;
    }
    return ir;
  }

  /// A `bufferChannels`-wide buffer carrying `inputChannels` distinct ramps;
  /// extra channels repeat the last input channel like the speakers up-mix.
  std::shared_ptr<DSPAudioBuffer> inputWithChannels(int inputChannels, int bufferChannels) {
    auto input = std::make_shared<DSPAudioBuffer>(FRAMES, bufferChannels, sampleRate);
    for (int c = 0; c < bufferChannels; ++c) {
      const int source = std::min(c, inputChannels - 1);
      for (size_t i = 0; i < FRAMES; ++i) {
        (*input->getChannel(c))[i] = static_cast<float>(i + 1) * static_cast<float>(source + 1);
      }
    }
    return input;
  }

  static float ramp(int inputChannel, size_t frame, size_t delay) {
    return frame >= delay
        ? static_cast<float>(frame - delay + 1) * static_cast<float>(inputChannel + 1)
        : 0.0f;
  }
};

class TestableConvolverNode : public ConvolverNode {
 public:
  explicit TestableConvolverNode(std::shared_ptr<BaseAudioContext> context)
      : ConvolverNode(context, ConvolverOptions()) {}

  /// Plays the graph's part: negotiate the width for `inputChannels` and
  /// install a buffer of that width.
  size_t negotiate(size_t inputChannels) {
    const size_t bufferChannels = negotiateBufferChannelCount(inputChannels);
    audioBuffer_ = std::make_shared<DSPAudioBuffer>(
        FRAMES_PER_QUANTUM, static_cast<int>(bufferChannels), getContextSampleRate());
    return bufferChannels;
  }

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &input) {
    audioBuffer_ = input;
  }

  /// Mirrors ConvolverNodeHostObject::setBuffer without normalization.
  void loadImpulseResponse(const std::shared_ptr<AudioBuffer> &ir) {
    const size_t irChannels = ir->getNumberOfChannels();

    std::vector<std::unique_ptr<Convolver>> convolvers;
    for (size_t channel = 0; channel < irChannels; ++channel) {
      convolvers.push_back(makeConvolver(*ir, channel));
    }

    auto internalBuffer =
        std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE * 2, 2, ir->getSampleRate());
    auto intermediateBuffer = std::make_shared<DSPAudioBuffer>(
        RENDER_QUANTUM_SIZE,
        static_cast<int>(std::max<size_t>(irChannels, 2)),
        ir->getSampleRate());

    setImpulseResponseForNegotiation(ir, irChannels);
    setBuffer(
        ir,
        std::move(convolvers),
        std::make_shared<ConvolverThreadPool>(2),
        internalBuffer,
        intermediateBuffer,
        1.0f);
  }

  void processNode(int framesToProcess) override {
    ConvolverNode::processNode(framesToProcess);
  }

  void mixInputs(const std::vector<const DSPAudioBuffer *> &inputs) override {
    ConvolverNode::mixInputs(inputs);
  }

  using AudioNode::setChannelInterpretation;

 private:
  static constexpr int FRAMES_PER_QUANTUM = RENDER_QUANTUM_SIZE;
};

TEST_F(ConvolverTest, ConvolverCanBeCreated) {
  auto convolver = std::make_shared<ConvolverNode>(context, ConvolverOptions());
  ASSERT_NE(convolver, nullptr);
}

TEST_F(ConvolverTest, DefaultChannelCountModeIsClampedMax) {
  ConvolverOptions defaults;
  EXPECT_EQ(defaults.channelCountMode, ChannelCountMode::CLAMPED_MAX);

  AudioNodeOptions unspecified;
  EXPECT_EQ(ConvolverOptions(unspecified).channelCountMode, ChannelCountMode::CLAMPED_MAX);

  AudioNodeOptions explicitMode;
  explicitMode.channelCountMode = ChannelCountMode::EXPLICIT;
  EXPECT_EQ(ConvolverOptions(explicitMode).channelCountMode, ChannelCountMode::EXPLICIT);
}

TEST_F(ConvolverTest, NegotiatesOutputWidthFromInputAndImpulseResponse) {
  TestableConvolverNode convolver(context);
  EXPECT_EQ(convolver.getUpstreamChannelCount(1), 1u);
  EXPECT_EQ(convolver.getUpstreamChannelCount(2), 1u); // no IR: one channel of silence

  convolver.loadImpulseResponse(impulseResponseWithTaps({1}));
  EXPECT_EQ(convolver.negotiate(1), 1u);
  EXPECT_EQ(convolver.negotiate(2), 2u);
  EXPECT_EQ(convolver.getUpstreamChannelCount(1), 1u);
  EXPECT_EQ(convolver.getUpstreamChannelCount(2), 2u);

  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  EXPECT_EQ(convolver.negotiate(1), 2u);
  EXPECT_EQ(convolver.getUpstreamChannelCount(1), 2u);

  convolver.loadImpulseResponse(impulseResponseWithTaps({0, 1, 2, 3}));
  EXPECT_EQ(convolver.negotiate(1), 2u);
  EXPECT_EQ(convolver.negotiate(2), 2u);
}

TEST_F(ConvolverTest, WithoutImpulseResponseOutputsSilence) {
  TestableConvolverNode convolver(context);
  ASSERT_EQ(convolver.negotiate(2), 1u);
  convolver.setInputBuffer(inputWithChannels(1, 1));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  ASSERT_EQ(output->getNumberOfChannels(), 1u);
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_FLOAT_EQ((*output->getChannel(0))[i], 0.0f);
  }
}

TEST_F(ConvolverTest, MonoInputWithMonoResponseOutputsMono) {
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1}));
  ASSERT_EQ(convolver.negotiate(1), 1u);
  convolver.setInputBuffer(inputWithChannels(1, 1));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  ASSERT_EQ(output->getNumberOfChannels(), 1u);
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 1), 1e-3);
  }
}

TEST_F(ConvolverTest, MonoInputWithStereoResponseOutputsStereo) {
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  ASSERT_EQ(convolver.negotiate(1), 2u);
  convolver.setInputBuffer(inputWithChannels(1, 2));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  ASSERT_EQ(output->getNumberOfChannels(), 2u);
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 1), 1e-3);
    EXPECT_NEAR((*output->getChannel(1))[i], ramp(0, i, 2), 1e-3);
  }
}

TEST_F(ConvolverTest, MonoResponseWithStereoBufferBeforeSecondConvolverArrivesStaysInBounds) {
  // The wider buffer comes through a negotiation event and the second
  // convolver through a later audio event; in between only the left channel
  // can be rendered. Installing the stereo buffer without negotiating models
  // that window (an idle offline context would otherwise deliver the event
  // synchronously).
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1}));
  convolver.setInputBuffer(inputWithChannels(2, 2));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 1), 1e-3);
    EXPECT_FLOAT_EQ((*output->getChannel(1))[i], 0.0f);
  }
}

TEST_F(ConvolverTest, StereoInputWithMonoResponseConvolvesEachChannel) {
  // A mono response is installed with one convolver; negotiating a stereo
  // input schedules the second, which an idle offline context runs at once.
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1}));
  ASSERT_EQ(convolver.negotiate(2), 2u);
  convolver.setInputBuffer(inputWithChannels(2, 2));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 1), 1e-3);
    EXPECT_NEAR((*output->getChannel(1))[i], ramp(1, i, 1), 1e-3);
  }
}

TEST_F(ConvolverTest, StereoInputWithStereoResponseHasNoCrossTerms) {
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  ASSERT_EQ(convolver.negotiate(2), 2u);
  convolver.setInputBuffer(inputWithChannels(2, 2));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 1), 1e-3);
    EXPECT_NEAR((*output->getChannel(1))[i], ramp(1, i, 2), 1e-3);
  }
}

TEST_F(ConvolverTest, StereoInputWithTrueStereoResponseFollowsSpecMatrix) {
  TestableConvolverNode convolver(context);
  // L -> IR0 -> L, L -> IR1 -> R, R -> IR2 -> L, R -> IR3 -> R
  convolver.loadImpulseResponse(impulseResponseWithTaps({0, 1, 2, 3}));
  ASSERT_EQ(convolver.negotiate(2), 2u);
  convolver.setInputBuffer(inputWithChannels(2, 2));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 0) + ramp(1, i, 2), 1e-3);
    EXPECT_NEAR((*output->getChannel(1))[i], ramp(0, i, 1) + ramp(1, i, 3), 1e-3);
  }
}

TEST_F(ConvolverTest, MonoInputWithTrueStereoResponseFeedsEveryChannel) {
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({0, 1, 2, 3}));
  ASSERT_EQ(convolver.negotiate(1), 2u);
  convolver.setInputBuffer(inputWithChannels(1, 2));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 0) + ramp(0, i, 2), 1e-3);
    EXPECT_NEAR((*output->getChannel(1))[i], ramp(0, i, 1) + ramp(0, i, 3), 1e-3);
  }
}

TEST_F(ConvolverTest, MonoComputedInputMixesMultichannelSourceToMonoThenDuplicates) {
  // channelCount 1 (explicit) with a quad source and a stereo IR: the source
  // must fold down with the spec's quad->mono gains (0.25 each) and that mono
  // signal must reach both buffer channels.
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  ASSERT_EQ(convolver.negotiate(1), 2u);

  auto quad = std::make_shared<DSPAudioBuffer>(FRAMES, 4, sampleRate);
  for (int c = 0; c < 4; ++c) {
    for (size_t i = 0; i < FRAMES; ++i) {
      (*quad->getChannel(c))[i] = static_cast<float>(c + 1);
    }
  }
  convolver.mixInputs({quad.get()});

  auto mixed = convolver.getOutputBuffer();
  ASSERT_EQ(mixed->getNumberOfChannels(), 2u);
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*mixed->getChannel(0))[i], 0.25f * (1 + 2 + 3 + 4), 1e-5);
    EXPECT_NEAR((*mixed->getChannel(1))[i], 0.25f * (1 + 2 + 3 + 4), 1e-5);
  }
}

TEST_F(ConvolverTest, MonoComputedInputUnderDiscreteInterpretationKeepsChannelZero) {
  TestableConvolverNode convolver(context);
  convolver.setChannelInterpretation(ChannelInterpretation::DISCRETE);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  ASSERT_EQ(convolver.negotiate(1), 2u);

  auto stereo = inputWithChannels(2, 2);
  convolver.mixInputs({stereo.get()});

  auto mixed = convolver.getOutputBuffer();
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_FLOAT_EQ((*mixed->getChannel(0))[i], ramp(0, i, 0));
    EXPECT_FLOAT_EQ((*mixed->getChannel(1))[i], ramp(0, i, 0));
  }
}

TEST_F(ConvolverTest, StereoComputedInputUsesDefaultMixing) {
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  ASSERT_EQ(convolver.negotiate(2), 2u);

  auto stereo = inputWithChannels(2, 2);
  convolver.mixInputs({stereo.get()});

  auto mixed = convolver.getOutputBuffer();
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_FLOAT_EQ((*mixed->getChannel(0))[i], ramp(0, i, 0));
    EXPECT_FLOAT_EQ((*mixed->getChannel(1))[i], ramp(1, i, 0));
  }
}

TEST_F(ConvolverTest, StereoResponseWithNotYetWidenedBufferStaysInBounds) {
  // The IR lands through an audio event and the wider buffer through a
  // negotiation event; for a quantum the buffer may still be mono.
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  convolver.setInputBuffer(inputWithChannels(1, 1));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  ASSERT_EQ(output->getNumberOfChannels(), 1u);
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_NEAR((*output->getChannel(0))[i], ramp(0, i, 1), 1e-3);
  }
}

TEST_F(ConvolverTest, ClearingImpulseResponseReturnsToMonoSilence) {
  TestableConvolverNode convolver(context);
  convolver.loadImpulseResponse(impulseResponseWithTaps({1, 2}));
  ASSERT_EQ(convolver.negotiate(2), 2u);
  convolver.setInputBuffer(inputWithChannels(2, 2));
  convolver.processNode(FRAMES);

  convolver.setImpulseResponseForNegotiation(nullptr, 0);
  convolver.clearBuffer();
  ASSERT_EQ(convolver.negotiate(2), 1u);
  convolver.setInputBuffer(inputWithChannels(1, 1));
  convolver.processNode(FRAMES);

  auto output = convolver.getOutputBuffer();
  ASSERT_EQ(output->getNumberOfChannels(), 1u);
  for (size_t i = 0; i < FRAMES; ++i) {
    EXPECT_FLOAT_EQ((*output->getChannel(0))[i], 0.0f);
  }
  EXPECT_EQ(convolver.getUpstreamChannelCount(2), 1u);
}

// NOLINTEND
