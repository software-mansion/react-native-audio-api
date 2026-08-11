#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/channel_splitter/ChannelSplitterInputNode.h>
#include <audioapi/core/effects/channel_splitter/ChannelSplitterOutputNode.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {
constexpr int kSampleRate = 44100;
constexpr int kFrames = 128;
} // namespace

class ChannelSplitterTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * kSampleRate, kSampleRate, eventRegistry);
  }
};

class TestableSplitterInput : public ChannelSplitterInputNode {
 public:
  using ChannelSplitterInputNode::ChannelSplitterInputNode;

  std::shared_ptr<DSPAudioBuffer> bus() {
    return getOutputBuffer();
  }

  void processWith(const std::vector<const DSPAudioBuffer *> &inputs, int frames) {
    processInputs(inputs, frames);
  }
};

class TestableSplitterOutput : public ChannelSplitterOutputNode {
 public:
  using ChannelSplitterOutputNode::ChannelSplitterOutputNode;

  std::shared_ptr<DSPAudioBuffer> outputBuffer() {
    return getOutputBuffer();
  }

  void process(int frames) {
    // Mirror the base pipeline: the mono buffer is zeroed before the channel
    // copy so inactive outputs stay silent.
    getInputBuffer()->zero();
    processNode(frames);
  }
};

TEST_F(ChannelSplitterTest, InputIsNotMixable) {
  TestableSplitterInput input(context, 4);
  EXPECT_EQ(input.getOutput(), nullptr);
  EXPECT_EQ(input.getOutputBuffer()->getNumberOfChannels(), 4u);
}

TEST_F(ChannelSplitterTest, InputMapsSourceChannelsDiscretely) {
  constexpr int channels = 4;
  TestableSplitterInput input(context, channels);

  auto source = std::make_shared<DSPAudioBuffer>(kFrames, channels, kSampleRate);
  for (int c = 0; c < channels; ++c) {
    for (int f = 0; f < kFrames; ++f) {
      (*source->getChannel(c))[f] = static_cast<float>(c + 1);
    }
  }

  std::vector<const DSPAudioBuffer *> inputs{source.get()};
  input.processWith(inputs, kFrames);

  auto bus = input.bus();
  for (int c = 0; c < channels; ++c) {
    for (int f = 0; f < kFrames; ++f) {
      EXPECT_FLOAT_EQ((*bus->getChannel(c))[f], static_cast<float>(c + 1));
    }
  }
}

TEST_F(ChannelSplitterTest, OutputExtractsItsChannel) {
  constexpr int channels = 4;
  auto bus = std::make_shared<DSPAudioBuffer>(kFrames, channels, kSampleRate);
  for (int c = 0; c < channels; ++c) {
    for (int f = 0; f < kFrames; ++f) {
      (*bus->getChannel(c))[f] = static_cast<float>(c + 10);
    }
  }

  for (int c = 0; c < channels; ++c) {
    TestableSplitterOutput output(context, bus, static_cast<size_t>(c));
    output.process(kFrames);

    auto out = output.outputBuffer();
    EXPECT_EQ(out->getNumberOfChannels(), 1u);
    for (int f = 0; f < kFrames; ++f) {
      EXPECT_FLOAT_EQ((*out->getChannel(0))[f], static_cast<float>(c + 10));
    }
  }
}

TEST_F(ChannelSplitterTest, InactiveOutputIsSilent) {
  // Bus narrower than the output's channel index — the output must emit silence.
  auto bus = std::make_shared<DSPAudioBuffer>(kFrames, 2, kSampleRate);
  for (int f = 0; f < kFrames; ++f) {
    (*bus->getChannel(0))[f] = 1.0f;
    (*bus->getChannel(1))[f] = 2.0f;
  }

  TestableSplitterOutput output(context, bus, 3);
  output.process(kFrames);

  auto out = output.outputBuffer();
  for (int f = 0; f < kFrames; ++f) {
    EXPECT_FLOAT_EQ((*out->getChannel(0))[f], 0.0f);
  }
}

// NOLINTEND
