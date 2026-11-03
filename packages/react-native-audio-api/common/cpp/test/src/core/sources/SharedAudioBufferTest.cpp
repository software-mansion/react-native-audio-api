#include <audioapi/core/sources/SharedAudioBuffer.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN
static constexpr size_t BUF_SIZE = 8;
static constexpr float SR = 44100.0f;

class SharedAudioBufferTest : public ::testing::Test {
 protected:
  std::shared_ptr<AudioBuffer> samples = std::make_shared<AudioBuffer>(BUF_SIZE, 2, SR);
  SharedAudioBuffer buffer{samples};

  void SetUp() override {
    fill(*samples, 0, 0.5f);
    fill(*samples, 1, 0.25f);
  }

  static void fill(AudioBuffer &target, size_t channel, float value) {
    for (size_t i = 0; i < target.getSize(); ++i) {
      (*target.getChannel(channel))[i] = value;
    }
  }

  void writeToChannel(size_t channel, float value, double startInChannel = 0) {
    std::vector<float> source(BUF_SIZE, value);
    buffer.copyToChannel(source.data(), source.size(), channel, startInChannel);
  }

  static void expectChannel(const AudioBuffer &target, size_t channel, float value) {
    for (size_t i = 0; i < target.getSize(); ++i) {
      EXPECT_FLOAT_EQ((*target.getChannel(channel))[i], value)
          << "channel " << channel << " frame " << i;
    }
  }
};

TEST_F(SharedAudioBufferTest, AcquireContentSharesStorageInsteadOfCopying) {
  auto content = buffer.acquireContent();

  EXPECT_EQ(content->getSharedChannel(0), samples->getSharedChannel(0));
  EXPECT_EQ(content->getSharedChannel(1), samples->getSharedChannel(1));
}

TEST_F(SharedAudioBufferTest, CopyToChannelAfterAcquireDoesNotReachThePlayedContent) {
  auto content = buffer.acquireContent();

  writeToChannel(0, 1.0f);

  expectChannel(*content, 0, 0.5f);
  expectChannel(*buffer.getSamples(), 0, 1.0f);
  EXPECT_EQ(content->getSharedChannel(1), buffer.getSamples()->getSharedChannel(1))
      << "Only the written channel is copied.";
}

TEST_F(SharedAudioBufferTest, CopyToChannelWithoutAcquireWritesInPlace) {
  auto storageBefore = samples->getSharedChannel(0);

  writeToChannel(0, 1.0f);

  EXPECT_EQ(samples->getSharedChannel(0), storageBefore);
  expectChannel(*samples, 0, 1.0f);
}

TEST_F(SharedAudioBufferTest, WritesThroughAnExposedViewBeforeAcquireArePlayed) {
  auto view = buffer.exposeChannel(0);
  (*view)[2] = 1.0f;

  auto content = buffer.acquireContent();

  EXPECT_FLOAT_EQ((*content->getChannel(0))[2], 1.0f);
}

TEST_F(SharedAudioBufferTest, AcquireContentCutsOffExposedViews) {
  auto view = buffer.exposeChannel(0);
  auto content = buffer.acquireContent();

  (*view)[2] = 1.0f;

  expectChannel(*content, 0, 0.5f);
  expectChannel(*buffer.getSamples(), 0, 0.5f);
  EXPECT_NE(buffer.exposeChannel(0), view) << "The next view must point to fresh storage.";
}

TEST_F(SharedAudioBufferTest, ExposingAChannelAfterAcquireDoesNotHandOutPlayedStorage) {
  auto content = buffer.acquireContent();

  auto view = buffer.exposeChannel(1);
  (*view)[0] = 1.0f;

  expectChannel(*content, 1, 0.25f);
}

TEST_F(SharedAudioBufferTest, ExposingTheSameChannelTwiceReusesItsStorage) {
  auto first = buffer.exposeChannel(0);

  EXPECT_EQ(buffer.exposeChannel(0), first);
}

TEST_F(SharedAudioBufferTest, OutOfRangeStartCopiesNothing) {
  writeToChannel(0, 1.0f, -1);
  writeToChannel(0, 1.0f, BUF_SIZE);

  expectChannel(*samples, 0, 0.5f);

  std::vector<float> destination(BUF_SIZE, -1.0f);
  buffer.copyFromChannel(destination.data(), destination.size(), 0, -1);
  buffer.copyFromChannel(destination.data(), destination.size(), 0, BUF_SIZE);

  EXPECT_EQ(destination, std::vector<float>(BUF_SIZE, -1.0f));
}

TEST_F(SharedAudioBufferTest, CopiesPastTheEndOfTheChannelAreTruncated) {
  writeToChannel(0, 1.0f, BUF_SIZE - 2);

  std::vector<float> destination(BUF_SIZE, -1.0f);
  buffer.copyFromChannel(destination.data(), destination.size(), 0, BUF_SIZE - 3);

  EXPECT_FLOAT_EQ(destination[0], 0.5f);
  EXPECT_FLOAT_EQ(destination[1], 1.0f);
  EXPECT_FLOAT_EQ(destination[2], 1.0f);
  EXPECT_FLOAT_EQ(destination[3], -1.0f) << "Frames past the channel end stay untouched.";
}
// NOLINTEND
