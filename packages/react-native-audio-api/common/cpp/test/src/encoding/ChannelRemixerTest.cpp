#include <audioapi/encoding/ChannelRemixer.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr size_t kFrames = 64;
constexpr float kSampleRate = 48000.0f;
const float kSqrtHalf = std::sqrt(0.5f);

/// A kFrames-long buffer holding one constant level per channel.
AudioBuffer constantInput(const std::vector<float> &levels) {
  AudioBuffer input(kFrames, static_cast<int>(levels.size()), kSampleRate);
  for (size_t channel = 0; channel < levels.size(); ++channel) {
    std::fill_n(input.getChannel(channel)->begin(), kFrames, levels[channel]);
  }
  return input;
}

/// Remixes one constant level per input channel and returns each output channel's level.
std::vector<float> remixLevels(const std::vector<float> &inputLevels, int outputChannelCount) {
  const AudioBuffer input = constantInput(inputLevels);
  ChannelRemixer remixer(
      static_cast<int>(inputLevels.size()), outputChannelCount, kFrames, kSampleRate);
  const AudioBuffer &remixed = remixer.remix(input, kFrames);

  std::vector<float> levels;
  for (int channel = 0; channel < outputChannelCount; ++channel) {
    levels.push_back((*remixed.getChannel(channel))[kFrames / 2]);
  }
  return levels;
}

} // namespace

TEST(ChannelRemixerTest, MatchingCountsPassTheCallersChannelsThrough) {
  const AudioBuffer input = constantInput({0.1f, 0.2f});

  ChannelRemixer remixer(2, 2, kFrames, kSampleRate);
  const AudioBuffer &remixed = remixer.remix(input, kFrames);

  EXPECT_FALSE(remixer.isRemixing());
  EXPECT_EQ(&remixed, &input);
}

TEST(ChannelRemixerTest, FiveOneToStereoMixesCenterAndSurroundsAndDropsLfe) {
  const float left = 0.1f, right = 0.2f, center = 0.3f, lfe = 0.4f;
  const float surroundLeft = 0.05f, surroundRight = 0.06f;
  const auto levels = remixLevels({left, right, center, lfe, surroundLeft, surroundRight}, 2);

  EXPECT_NEAR(levels[0], left + kSqrtHalf * (center + surroundLeft), 1e-6f);
  EXPECT_NEAR(levels[1], right + kSqrtHalf * (center + surroundRight), 1e-6f);
}

TEST(ChannelRemixerTest, QuadToStereoAveragesFrontAndSurroundPerSide) {
  const auto levels = remixLevels({0.1f, 0.2f, 0.3f, 0.4f}, 2);

  EXPECT_NEAR(levels[0], 0.5f * (0.1f + 0.3f), 1e-6f);
  EXPECT_NEAR(levels[1], 0.5f * (0.2f + 0.4f), 1e-6f);
}

TEST(ChannelRemixerTest, StereoToMonoAveragesBothSides) {
  const auto levels = remixLevels({0.2f, 0.6f}, 1);

  EXPECT_NEAR(levels[0], 0.4f, 1e-6f);
}

TEST(ChannelRemixerTest, CountWithoutSpeakerRuleKeepsLeadingChannelsInOrder) {
  std::vector<float> inputLevels;
  for (int channel = 0; channel < 16; ++channel) {
    inputLevels.push_back(0.01f * static_cast<float>(channel + 1));
  }
  const auto levels = remixLevels(inputLevels, 2);

  EXPECT_NEAR(levels[0], 0.01f, 1e-6f);
  EXPECT_NEAR(levels[1], 0.02f, 1e-6f);
}

TEST(ChannelRemixerTest, RemixingReplacesThePreviousBlock) {
  ChannelRemixer remixer(2, 1, kFrames, kSampleRate);
  (void)remixer.remix(constantInput({0.5f, 0.5f}), kFrames);
  const AudioBuffer &remixed = remixer.remix(constantInput({0.1f, 0.1f}), kFrames);

  EXPECT_NEAR((*remixed.getChannel(0))[0], 0.1f, 1e-6f);
}

// NOLINTEND
