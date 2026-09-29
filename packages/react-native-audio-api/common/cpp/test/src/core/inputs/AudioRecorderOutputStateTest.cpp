#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <string>

using namespace audioapi;

// NOLINTBEGIN

namespace {

/// Never starts a stream, so every output it is given stays at Requested at most.
class IdleAudioRecorder : public AudioRecorder {
 public:
  IdleAudioRecorder() : AudioRecorder(nullptr) {}

  using AudioRecorder::deactivate;
  using AudioRecorder::OutputState;
  using AudioRecorder::wantsCallback;
  using AudioRecorder::wantsFileOutput;

  Result<NoneType, std::string> start() override {
    return Err(std::string("not supported"));
  }
  Result<FileInfo, std::string> stop() override {
    return Err(std::string("not supported"));
  }
  void pause() override {}
  void resume() override {}
  bool isRecording() const override {
    return false;
  }
  bool isPaused() const override {
    return false;
  }
  bool isIdle() const override {
    return true;
  }
  [[nodiscard]] double getInputLatency() const override {
    return 0.0;
  }

 protected:
  [[nodiscard]] Result<StreamFormat, std::string> resolveStreamFormat() const override {
    return Err(std::string("no stream"));
  }
};

std::shared_ptr<AudioFileProperties> wavProperties() {
  return std::make_shared<AudioFileProperties>(
      AudioFileProperties::PathConfig{
          .directory = AudioFileProperties::FileDirectory::Cache,
          .subDirectory = "",
          .fileName = "",
      },
      AudioFileProperties::StreamConfig{.sampleRate = 48000.0F, .channelCount = 1},
      AudioFileProperties::EncodingConfig{
          .format = AudioFileProperties::FileFormat::WAV,
          .bitRate = 0,
          .bitDepth = AudioFileProperties::BitDepth::Bit16,
          .flacCompressionLevel = 0,
          .iosAudioQuality = AudioFileProperties::IOSAudioQuality::High,
      },
      AudioFileProperties::WriterConfig{.rotateIntervalBytes = 0, .androidFlushIntervalMs = 0});
}

} // namespace

using OutputState = IdleAudioRecorder::OutputState;

TEST(AudioRecorderOutputStateTest, DeactivateDemotesAnActiveOutputToRequested) {
  std::atomic<OutputState> state{OutputState::Active};
  IdleAudioRecorder::deactivate(state);
  EXPECT_EQ(state.load(), OutputState::Requested);
}

TEST(AudioRecorderOutputStateTest, DeactivateNeverEnablesADisabledOutput) {
  std::atomic<OutputState> state{OutputState::Disabled};
  IdleAudioRecorder::deactivate(state);
  EXPECT_EQ(state.load(), OutputState::Disabled);
}

TEST(AudioRecorderOutputStateTest, InvalidFilePropertiesAreRejectedAtEnable) {
  IdleAudioRecorder recorder;
  auto properties = wavProperties();
  properties->stream.sampleRate = 0.0F;

  EXPECT_TRUE(recorder.enableFileOutput(properties).is_err());
  EXPECT_FALSE(recorder.wantsFileOutput());
}

TEST(AudioRecorderOutputStateTest, AFormatThePlatformCannotWriteIsRejectedAtEnable) {
  IdleAudioRecorder recorder;
  auto properties = wavProperties();
  properties->encoding.format = AudioFileProperties::FileFormat::OPUS_WEBM;

  EXPECT_TRUE(recorder.enableFileOutput(properties).is_err());
  EXPECT_FALSE(recorder.wantsFileOutput());
}

TEST(AudioRecorderOutputStateTest, CallbackIsRequestedUntilAStreamPreparesIt) {
  IdleAudioRecorder recorder;

  ASSERT_TRUE(recorder.setOnAudioReadyCallback(48000.0F, 1024, 1, 1).is_ok());
  EXPECT_TRUE(recorder.wantsCallback());
  EXPECT_FALSE(recorder.usesCallback());

  recorder.clearOnAudioReadyCallback();
  EXPECT_FALSE(recorder.wantsCallback());
}

// NOLINTEND
