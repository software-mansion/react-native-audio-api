#include <audioapi/encoding/AudioEncoder.h>
#include <gtest/gtest.h>

#include <string>
#include <vector>

using namespace audioapi;

// NOLINTBEGIN

namespace {

/// Exercises the duration accounting the base class keeps for every platform encoder.
class CountingEncoder : public AudioEncoder {
 public:
  CountingEncoder() : AudioEncoder(EncoderSettings{}) {}

  OpenEncoderResult open(
      const StreamFormat &inputFormat,
      const EncoderOutputSpec & /*outputSpec*/,
      const std::string &filePath) override {
    inputFormat_ = inputFormat;
    filePath_ = filePath;
    resetEncodedDuration();
    markOpen();
    return OpenEncoderResult::Ok(filePath);
  }

  EncodeResult encode(const float *const * /*channels*/, int numFrames) override {
    addEncodedFrames(static_cast<size_t>(numFrames));
    return EncodeResult::Ok(static_cast<size_t>(numFrames));
  }

  CloseEncoderResult close() override {
    markClosed();
    return CloseEncoderResult::Ok({0.0, getEncodedDurationSeconds()});
  }

  [[nodiscard]] size_t getFileSizeBytes() const override {
    return 0;
  }
};

/// Mirrors IOSEncoder: the one encoder that keeps its file across an input format change.
class ChangeableEncoder final : public CountingEncoder {
 protected:
  OpenEncoderResult reprepareInput(const StreamFormat &inputFormat) override {
    inputFormat_ = inputFormat;
    return OpenEncoderResult::Ok(getFilePath());
  }
};

const EncoderOutputSpec WAV_SPEC{
    .container = AudioContainer::WAV,
    .codec = AudioCodec::PCM,
    .extension = "wav"};

StreamFormat monoAt(float sampleRate) {
  return StreamFormat{
      .layout = {.sampleRate = sampleRate, .channelCount = 1}, .maxFramesPerBuffer = 4800};
}

void encodeFrames(AudioEncoder &encoder, int numFrames) {
  std::vector<float> samples(static_cast<size_t>(numFrames), 0.0F);
  const float *channels[] = {samples.data()};
  ASSERT_TRUE(encoder.encode(channels, numFrames).is_ok());
}

} // namespace

TEST(AudioEncoderTest, CountsEncodedFramesAtTheInputRate) {
  CountingEncoder encoder;
  ASSERT_TRUE(encoder.open(monoAt(48000.0F), WAV_SPEC, "count.wav").is_ok());

  encodeFrames(encoder, 4800);

  EXPECT_DOUBLE_EQ(encoder.getEncodedDurationSeconds(), 0.1);
}

TEST(AudioEncoderTest, AFormatChangeKeepsEarlierAudioAtItsOwnRate) {
  ChangeableEncoder encoder;
  ASSERT_TRUE(encoder.open(monoAt(48000.0F), WAV_SPEC, "retarget.wav").is_ok());

  encodeFrames(encoder, 4800);
  ASSERT_TRUE(encoder.changeInputFormat(monoAt(24000.0F)).is_ok());
  encodeFrames(encoder, 2400);

  EXPECT_DOUBLE_EQ(encoder.getEncodedDurationSeconds(), 0.2);
}

TEST(AudioEncoderTest, ARefusedFormatChangeLeavesTheCountUntouched) {
  CountingEncoder encoder;
  ASSERT_TRUE(encoder.open(monoAt(48000.0F), WAV_SPEC, "refused.wav").is_ok());

  encodeFrames(encoder, 4800);
  EXPECT_TRUE(encoder.changeInputFormat(monoAt(24000.0F)).is_err());
  encodeFrames(encoder, 4800);

  EXPECT_DOUBLE_EQ(encoder.getEncodedDurationSeconds(), 0.2);
}

TEST(AudioEncoderTest, ReopeningStartsTheCountFromZero) {
  ChangeableEncoder encoder;
  ASSERT_TRUE(encoder.open(monoAt(48000.0F), WAV_SPEC, "first.wav").is_ok());
  encodeFrames(encoder, 4800);
  ASSERT_TRUE(encoder.changeInputFormat(monoAt(24000.0F)).is_ok());
  ASSERT_TRUE(encoder.close().is_ok());

  ASSERT_TRUE(encoder.open(monoAt(48000.0F), WAV_SPEC, "second.wav").is_ok());

  EXPECT_DOUBLE_EQ(encoder.getEncodedDurationSeconds(), 0.0);
}

// NOLINTEND
