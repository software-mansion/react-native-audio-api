#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/channel_merger/ChannelMergerInputNode.h>
#include <audioapi/core/effects/channel_merger/ChannelMergerOutputNode.h>
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

class ChannelMergerTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(2, 5 * kSampleRate, kSampleRate, eventRegistry);
  }
};

class TestableMergerInput : public ChannelMergerInputNode {
 public:
  using ChannelMergerInputNode::ChannelMergerInputNode;

  void setInputBuffer(const std::shared_ptr<DSPAudioBuffer> &buffer) {
    audioBuffer_ = buffer;
  }

  void process(int frames) {
    processNode(frames);
  }
};

TEST_F(ChannelMergerTest, InputIsNotMixable) {
  auto bus = std::make_shared<DSPAudioBuffer>(kFrames, 4, kSampleRate);
  TestableMergerInput input(context, bus, 0);
  EXPECT_EQ(input.getOutput(), nullptr);
}

TEST_F(ChannelMergerTest, PacksEachInputIntoItsOwnChannel) {
  constexpr int channels = 4;
  auto bus = std::make_shared<DSPAudioBuffer>(kFrames, channels, kSampleRate);

  std::vector<std::unique_ptr<TestableMergerInput>> inputs;
  for (int i = 0; i < channels; ++i) {
    auto input = std::make_unique<TestableMergerInput>(context, bus, static_cast<size_t>(i));

    auto mono = std::make_shared<DSPAudioBuffer>(kFrames, 1, kSampleRate);
    for (int f = 0; f < kFrames; ++f) {
      (*mono->getChannel(0))[f] = static_cast<float>(i + 1);
    }
    input->setInputBuffer(mono);
    inputs.push_back(std::move(input));
  }

  for (auto &input : inputs) {
    input->process(kFrames);
  }

  for (int c = 0; c < channels; ++c) {
    for (int f = 0; f < kFrames; ++f) {
      EXPECT_FLOAT_EQ((*bus->getChannel(c))[f], static_cast<float>(c + 1));
    }
  }
}

TEST_F(ChannelMergerTest, OutputBufferHasNumberOfInputsChannels) {
  constexpr int channels = 6;
  ChannelMergerOutputNode output(context, channels);
  EXPECT_EQ(output.getOutputBuffer()->getNumberOfChannels(), static_cast<size_t>(channels));
}

// NOLINTEND
