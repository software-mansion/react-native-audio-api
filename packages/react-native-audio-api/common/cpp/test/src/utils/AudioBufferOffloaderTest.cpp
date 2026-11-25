#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioBufferOffloader.hpp>
#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr size_t FRAMES = 4;
constexpr StreamFormat STEREO{
    .layout = {.sampleRate = 48000.0F, .channelCount = 2},
    .maxFramesPerBuffer = 8,
};

} // namespace

TEST(AudioBufferOffloaderTest, RawChannelsAndBuffersBothReachTheWorker) {
  std::vector<std::vector<float>> delivered;
  AudioBufferOffloader offloader([&delivered](PendingFrames pending) {
    const auto &right = *pending.buffer->getChannel(1);
    delivered.emplace_back(right.begin(), right.begin() + pending.numFrames);
  });
  ASSERT_TRUE(offloader.start(STEREO));

  const std::vector<float> left(FRAMES, 1.0F);
  const std::vector<float> right(FRAMES, 2.0F);
  const float *const channels[] = {left.data(), right.data()};
  offloader.push(channels, FRAMES);

  AudioBuffer mono(FRAMES, 1, 48000.0F);
  for (size_t i = 0; i < FRAMES; ++i) {
    (*mono.getChannel(0))[i] = 3.0F;
  }
  offloader.push(mono, FRAMES);

  // Drains both to the callback and joins the worker.
  offloader.stop();

  ASSERT_EQ(delivered.size(), 2U);
  EXPECT_EQ(delivered[0], std::vector<float>(FRAMES, 2.0F));
  // Mono up-mixes into both channels.
  EXPECT_EQ(delivered[1], std::vector<float>(FRAMES, 3.0F));
}

TEST(AudioBufferOffloaderTest, PushIsDroppedWhenStoppedOrTooLong) {
  size_t deliveredCount = 0;
  AudioBufferOffloader offloader([&deliveredCount](PendingFrames) { ++deliveredCount; });
  const std::vector<float> samples(16, 0.5F);
  const float *const channels[] = {samples.data(), samples.data()};

  offloader.push(channels, FRAMES);
  ASSERT_TRUE(offloader.start(STEREO));
  offloader.push(channels, STEREO.maxFramesPerBuffer + 1);
  offloader.push(static_cast<const float *const *>(nullptr), FRAMES);
  offloader.stop();

  EXPECT_EQ(deliveredCount, 0U);
}

// NOLINTEND
