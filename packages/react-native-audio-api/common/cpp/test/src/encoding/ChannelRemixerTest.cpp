#include <audioapi/encoding/ChannelRemixer.h>
#include <gtest/gtest.h>

#include <cmath>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr size_t kFrames = 64;
constexpr float kSampleRate = 48000.0f;
const float kSqrtHalf = std::sqrt(0.5f);

/// Remixes one constant level per input channel and returns each output channel's level.
std::vector<float> remixLevels(const std::vector<float> &inputLevels, int outputChannelCount) {
  std::vector<std::vector<float>> planar;
  std::vector<const float *> channels;
  for (float level : inputLevels) {
    planar.emplace_back(kFrames, level);
  }
  for (const auto &channel : planar) {
    channels.push_back(channel.data());
  }

  ChannelRemixer remixer(
      static_cast<int>(inputLevels.size()), outputChannelCount, kFrames, kSampleRate);
  const auto remixed = remixer.remix(channels.data(), kFrames);

  std::vector<float> levels;
  for (int channel = 0; channel < outputChannelCount; ++channel) {
    levels.push_back(remixed[channel][kFrames / 2]);
  }
  return levels;
}

} // namespace

TEST(ChannelRemixerTest, MatchingCountsPassTheCallersChannelsThrough) {
  std::vector<float> left(kFrames, 0.1f);
  std::vector<float> right(kFrames, 0.2f);
  const float *channels[] = {left.data(), right.data()};

  ChannelRemixer remixer(2, 2, kFrames, kSampleRate);
  const auto remixed = remixer.remix(channels, kFrames);

  EXPECT_FALSE(remixer.isRemixing());
  EXPECT_EQ(remixed[0], left.data());
  EXPECT_EQ(remixed[1], right.data());
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
  std::vector<float> first(kFrames, 0.5f);
  std::vector<float> second(kFrames, 0.1f);
  const float *firstChannels[] = {first.data(), first.data()};
  const float *secondChannels[] = {second.data(), second.data()};

  ChannelRemixer remixer(2, 1, kFrames, kSampleRate);
  (void)remixer.remix(firstChannels, kFrames);
  const auto remixed = remixer.remix(secondChannels, kFrames);

  EXPECT_NEAR(remixed[0][0], 0.1f, 1e-6f);
}

// NOLINTEND
