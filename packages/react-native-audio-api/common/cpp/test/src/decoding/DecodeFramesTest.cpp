#include <audioapi/decoding/AudioDecoding.h>
#include <audioapi/decoding/backends/AudioDecoderBackend.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

namespace {

constexpr size_t unbounded = std::numeric_limits<size_t>::max();

/// A mono source of @p totalFrames frames whose samples count the frames, so a decoded buffer
/// shows which frames it holds. It can announce a length that is not its real one, and return
/// fewer frames per read than asked, as a real decoder may.
class CountingDecoder : public decoding::AudioDecoderBackend {
 public:
  CountingDecoder(size_t totalFrames, size_t announcedFrames, size_t maxFramesPerRead = unbounded)
      : totalFrames_(totalFrames), maxFramesPerRead_(maxFramesPerRead) {
    outputChannels_ = 1;
    outputSampleRate_ = 48000;
    totalPcmFrames_ = announcedFrames;
  }

  size_t readPcmFrames(float *outInterleaved, size_t frameCount) override {
    largestRequest_ = std::max(largestRequest_, frameCount);
    const size_t frames = std::min({frameCount, maxFramesPerRead_, totalFrames_ - position_});
    for (size_t i = 0; i < frames; ++i) {
      outInterleaved[i] = static_cast<float>(position_ + i);
    }
    position_ += frames;
    return frames;
  }

  decoding::DecoderResult seekToTime(double /*seconds*/) override {
    return Ok(None);
  }
  void close() override {}
  bool isOpen() const override {
    return true;
  }

  size_t largestRequest() const {
    return largestRequest_;
  }

 private:
  size_t totalFrames_;
  size_t maxFramesPerRead_;
  size_t position_ = 0;
  size_t largestRequest_ = 0;
};

void expectFrames(const std::shared_ptr<AudioBuffer> &buffer, size_t firstFrame, size_t count) {
  ASSERT_NE(buffer, nullptr);
  ASSERT_EQ(buffer->getSize(), count);
  const auto samples = buffer->getChannel(0)->span();
  for (size_t i = 0; i < count; ++i) {
    ASSERT_FLOAT_EQ(samples[i], static_cast<float>(firstFrame + i)) << "frame " << i;
  }
}

} // namespace

TEST(DecodeFramesTest, DecodesEverythingWhenTheLengthIsAnnounced) {
  CountingDecoder decoder(100000, 100000);

  expectFrames(audiodecoding::decodeFrames(decoder, unbounded), 0, 100000);
}

TEST(DecodeFramesTest, GrowsWhenTheLengthIsUnknown) {
  CountingDecoder decoder(200000, 0);

  expectFrames(audiodecoding::decodeFrames(decoder, unbounded), 0, 200000);
}

TEST(DecodeFramesTest, GrowsWhenTheSourceIsLongerThanAnnounced) {
  CountingDecoder decoder(50000, 1000);

  expectFrames(audiodecoding::decodeFrames(decoder, unbounded), 0, 50000);
}

TEST(DecodeFramesTest, ReturnsAShortSourceForALargeRequest) {
  CountingDecoder decoder(3000, 3000);

  expectFrames(audiodecoding::decodeFrames(decoder, 5000000), 0, 3000);
}

TEST(DecodeFramesTest, ReturnsAShortSourceThatAnnouncesTooMuch) {
  CountingDecoder decoder(3000, 10000000);

  expectFrames(audiodecoding::decodeFrames(decoder, 5000000), 0, 3000);
}

TEST(DecodeFramesTest, GrowsUpToALargeRequestWhenTheLengthIsUnknown) {
  CountingDecoder decoder(1000000, 0);

  expectFrames(audiodecoding::decodeFrames(decoder, 300000), 0, 300000);
}

TEST(DecodeFramesTest, StopsAtTheRequestAndContinuesFromThere) {
  CountingDecoder decoder(10000, 10000);

  expectFrames(audiodecoding::decodeFrames(decoder, 5000), 0, 5000);
  expectFrames(audiodecoding::decodeFrames(decoder, 3000), 5000, 3000);
  expectFrames(audiodecoding::decodeFrames(decoder, 5000), 8000, 2000);
  EXPECT_EQ(audiodecoding::decodeFrames(decoder, 5000), nullptr);
}

TEST(DecodeFramesTest, NeverAsksTheDecoderForMoreThanTheRequest) {
  CountingDecoder decoder(10000, 10000);

  expectFrames(audiodecoding::decodeFrames(decoder, 10), 0, 10);
  EXPECT_LE(decoder.largestRequest(), 10U);
}

TEST(DecodeFramesTest, KeepsReadingADecoderThatReturnsShortReads) {
  CountingDecoder decoder(10000, 10000, 100);

  expectFrames(audiodecoding::decodeFrames(decoder, 1000), 0, 1000);
  expectFrames(audiodecoding::decodeFrames(decoder, unbounded), 1000, 9000);
}

TEST(DecodeFramesTest, ReturnsNoBufferForAnEmptySource) {
  CountingDecoder decoder(0, 0);

  EXPECT_EQ(audiodecoding::decodeFrames(decoder, 4096), nullptr);
  EXPECT_EQ(audiodecoding::decodeFrames(decoder, unbounded), nullptr);
}

// NOLINTEND
