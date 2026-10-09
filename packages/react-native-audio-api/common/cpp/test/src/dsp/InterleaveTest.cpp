#include <audioapi/dsp/VectorMath.h>
#include <gtest/gtest.h>

#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

// Sample value that encodes its own position, so a transpose bug shows up as a
// wrong channel/frame pair rather than as plausible-looking audio.
float sampleFor(size_t channel, size_t frame) {
  return static_cast<float>(channel) * 1000.0F + static_cast<float>(frame);
}

std::vector<std::vector<float>> makePlanar(size_t channelCount, size_t frameCount) {
  std::vector<std::vector<float>> planar(channelCount, std::vector<float>(frameCount, 0.0F));
  for (size_t channel = 0; channel < channelCount; ++channel) {
    for (size_t frame = 0; frame < frameCount; ++frame) {
      planar[channel][frame] = sampleFor(channel, frame);
    }
  }
  return planar;
}

std::vector<const float *> constPointers(const std::vector<std::vector<float>> &planar) {
  std::vector<const float *> pointers;
  pointers.reserve(planar.size());
  for (const auto &channel : planar) {
    pointers.push_back(channel.data());
  }
  return pointers;
}

std::vector<float *> mutablePointers(std::vector<std::vector<float>> &planar) {
  std::vector<float *> pointers;
  pointers.reserve(planar.size());
  for (auto &channel : planar) {
    pointers.push_back(channel.data());
  }
  return pointers;
}

class InterleaveChannelCountTest : public ::testing::TestWithParam<size_t> {};

} // namespace

TEST_P(InterleaveChannelCountTest, InterleavePacksFramesInChannelOrder) {
  const size_t channelCount = GetParam();
  constexpr size_t frameCount = 259; // deliberately not a multiple of the block size

  auto planar = makePlanar(channelCount, frameCount);
  auto planarPointers = constPointers(planar);
  std::vector<float> interleaved(channelCount * frameCount, -1.0F);

  dsp::interleave(planarPointers.data(), channelCount, interleaved.data(), frameCount);

  for (size_t frame = 0; frame < frameCount; ++frame) {
    for (size_t channel = 0; channel < channelCount; ++channel) {
      EXPECT_FLOAT_EQ(interleaved[frame * channelCount + channel], sampleFor(channel, frame))
          << "channel " << channel << " frame " << frame;
    }
  }
}

TEST_P(InterleaveChannelCountTest, DeinterleaveIsTheInverseOfInterleave) {
  const size_t channelCount = GetParam();
  constexpr size_t frameCount = 259;

  auto planar = makePlanar(channelCount, frameCount);
  auto planarPointers = constPointers(planar);
  std::vector<float> interleaved(channelCount * frameCount, -1.0F);
  dsp::interleave(planarPointers.data(), channelCount, interleaved.data(), frameCount);

  auto roundTripped =
      std::vector<std::vector<float>>(channelCount, std::vector<float>(frameCount, -1.0F));
  auto roundTrippedPointers = mutablePointers(roundTripped);
  dsp::deinterleave(interleaved.data(), roundTrippedPointers.data(), channelCount, frameCount);

  EXPECT_EQ(roundTripped, planar);
}

INSTANTIATE_TEST_SUITE_P(
    ChannelCounts,
    InterleaveChannelCountTest,
    ::testing::Values(1U, 2U, 3U, 8U));

TEST(InterleaveTest, WritesNothingForZeroFrames) {
  auto planar = makePlanar(2, 4);
  auto planarPointers = constPointers(planar);
  std::vector<float> interleaved(8, 7.0F);

  dsp::interleave(planarPointers.data(), 2, interleaved.data(), 0);

  EXPECT_EQ(interleaved, std::vector<float>(8, 7.0F));
}

TEST(InterleaveTest, WritesNothingForZeroChannels) {
  std::vector<float> interleaved(8, 7.0F);

  dsp::interleave(nullptr, 0, interleaved.data(), 4);
  dsp::deinterleave(interleaved.data(), nullptr, 0, 4);

  EXPECT_EQ(interleaved, std::vector<float>(8, 7.0F));
}

TEST(InterleaveTest, TouchesOnlyTheRequestedFrames) {
  constexpr size_t channelCount = 3;
  constexpr size_t frameCount = 5;
  constexpr size_t requestedFrames = 2;

  auto planar = makePlanar(channelCount, frameCount);
  auto planarPointers = constPointers(planar);
  std::vector<float> interleaved(channelCount * frameCount, -1.0F);

  dsp::interleave(planarPointers.data(), channelCount, interleaved.data(), requestedFrames);

  for (size_t i = requestedFrames * channelCount; i < interleaved.size(); ++i) {
    EXPECT_FLOAT_EQ(interleaved[i], -1.0F) << "wrote past frame " << requestedFrames;
  }
}

// NOLINTEND
