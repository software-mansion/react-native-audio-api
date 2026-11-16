#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <test/src/utils/FakeFileBackend.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

using namespace audioapi;
using namespace audioapi::test_support;

// NOLINTBEGIN

namespace {

/// Never starts a stream: its outputs stay at Requested until the test prepares them for a
/// format by hand, as a platform recorder does when its stream opens or changes format.
class IdleAudioRecorder : public AudioRecorder {
 public:
  explicit IdleAudioRecorder(const PlatformFileBackend *fileBackend = nullptr)
      : AudioRecorder(nullptr, fileBackend) {}

  using AudioRecorder::OutputSlot;
  using AudioRecorder::OutputState;

  [[nodiscard]] bool wantsCallback() const {
    return callback_.isWanted();
  }
  [[nodiscard]] bool wantsConnection() const {
    return adapter_.isWanted();
  }
  [[nodiscard]] bool wantsFileOutput() const {
    return file_.isWanted();
  }

  Result<NoneType, std::string> prepareOutputsLocked(const StreamFormat &format) {
    std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
    return prepareOutputs(format);
  }

  Result<NoneType, std::string> reprepareOutputsLocked(const StreamFormat &format) {
    std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
    return reprepareOutputs(format);
  }

  /// Ends the session's outputs the way a platform stop() does.
  Result<FileInfo, std::string> finishOutputs() {
    DetachedSideEffects sideEffects;
    {
      std::scoped_lock outputsLock(callback_.mutex, file_.mutex, adapter_.mutex);
      sideEffects = detachSideEffects();
    }
    return finalizeSideEffects(std::move(sideEffects));
  }

  Result<NoneType, std::string> start() override {
    return Err(std::string("not supported"));
  }
  Result<FileInfo, std::string> stop() override {
    return Err(std::string("not supported"));
  }
  void pause() override {}
  Result<NoneType, std::string> resume() override {
    return Ok(None);
  }
  Result<NoneType, std::string> rerouteInput() override {
    return Ok(None);
  }
  bool isRecording() const override {
    return false;
  }
  bool isPaused() const override {
    return false;
  }
  bool isIdle() const override {
    return true;
  }

 protected:
  [[nodiscard]] double platformInputLatency(double /*callbackSeconds*/) const override {
    return 0.0;
  }

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
      AudioLayout{.sampleRate = 48000.0F, .channelCount = 1},
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
  IdleAudioRecorder::OutputSlot<AudioFileWriter> slot;
  slot.set(OutputState::Active);

  slot.deactivate();

  EXPECT_FALSE(slot.isActive());
  EXPECT_TRUE(slot.isWanted());
}

TEST(AudioRecorderOutputStateTest, DeactivateNeverEnablesADisabledOutput) {
  IdleAudioRecorder::OutputSlot<AudioFileWriter> slot;

  slot.deactivate();

  EXPECT_FALSE(slot.isWanted());
}

TEST(AudioRecorderOutputStateTest, InvalidFilePropertiesAreRejectedAtEnable) {
  IdleAudioRecorder recorder;
  auto properties = wavProperties();
  properties->fileLayout.sampleRate = 0.0F;

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

TEST(AudioRecorderOutputStateTest, AConnectionWhoseAdapterNodeIsGoneFailsToPrepare) {
  IdleAudioRecorder recorder;
  recorder.connect(std::make_shared<utils::graph::NodeHandle>(nullptr));
  ASSERT_TRUE(recorder.wantsConnection());

  auto result = recorder.prepareOutputsLocked(
      StreamFormat{
          .layout = AudioLayout{.sampleRate = 48000.0F, .channelCount = 1},
          .maxFramesPerBuffer = 1024,
      });

  EXPECT_TRUE(result.is_err());
  EXPECT_FALSE(recorder.isConnected());
  EXPECT_TRUE(recorder.wantsConnection());
}

namespace {

constexpr float kSampleRate = 48000.0F;

StreamFormat streamFormat(int channelCount) {
  return StreamFormat{
      .layout = AudioLayout{.sampleRate = kSampleRate, .channelCount = channelCount},
      .maxFramesPerBuffer = 1024,
  };
}

/// An adapter node that reports the channel count it was prepared for through buff_.
struct AdapterFixture {
  std::shared_ptr<OfflineAudioContext> context = std::make_shared<OfflineAudioContext>(
      2,
      512,
      kSampleRate,
      std::make_shared<MockAudioEventHandlerRegistry>());
  std::shared_ptr<utils::graph::NodeHandle> handle =
      std::make_shared<utils::graph::NodeHandle>(std::make_unique<RecorderAdapterNode>(context));

  [[nodiscard]] RecorderAdapterNode &adapter() const {
    return static_cast<RecorderAdapterNode &>(*handle->audioNode);
  }
};

} // namespace

TEST(AudioRecorderOutputStateTest, AnOutputThatCannotFollowAFormatChangeDoesNotHoldTheOthersBack) {
  FakeEncoderLog log;
  const auto backend = makeFakeBackend(log);
  AdapterFixture adapterFixture;
  IdleAudioRecorder recorder(&backend);
  ASSERT_TRUE(recorder.enableFileOutput(wavProperties()).is_ok());
  ASSERT_TRUE(recorder.setOnAudioReadyCallback(kSampleRate, 256, 1, 99).is_ok());
  recorder.connect(adapterFixture.handle);
  ASSERT_TRUE(recorder.prepareOutputsLocked(streamFormat(2)).is_ok());
  ASSERT_EQ(adapterFixture.adapter().buff_.size(), 2U);
  log.failNextReprepare.store(true);

  auto result = recorder.reprepareOutputsLocked(streamFormat(1));

  ASSERT_TRUE(result.is_err());
  EXPECT_NE(result.unwrap_err().find("encoder refused the new input format"), std::string::npos);
  EXPECT_FALSE(recorder.usesFileOutput());
  EXPECT_TRUE(recorder.wantsFileOutput());
  EXPECT_TRUE(recorder.usesCallback());
  EXPECT_TRUE(recorder.isConnected());
  EXPECT_EQ(adapterFixture.adapter().buff_.size(), 1U);
}

TEST(AudioRecorderOutputStateTest, AFileThatCannotFollowAFormatChangeIsStillReturned) {
  FakeEncoderLog log;
  const auto backend = makeFakeBackend(log);
  IdleAudioRecorder recorder(&backend);
  ASSERT_TRUE(recorder.enableFileOutput(wavProperties()).is_ok());
  ASSERT_TRUE(recorder.prepareOutputsLocked(streamFormat(2)).is_ok());
  log.failNextReprepare.store(true);
  ASSERT_TRUE(recorder.reprepareOutputsLocked(streamFormat(1)).is_err());

  auto recording = recorder.finishOutputs();

  ASSERT_TRUE(recording.is_ok());
  EXPECT_EQ(recording.unwrap().paths.size(), 1U);
  EXPECT_DOUBLE_EQ(recording.unwrap().duration, log.fileDurationSec);
  EXPECT_EQ(log.closedFiles.load(), 1);
}

namespace {

/// Records a session whose last buffer the test sets, and reports the platform's latency as
/// that buffer's duration plus a fixed amount.
class LiveAudioRecorder final : public IdleAudioRecorder {
 public:
  static constexpr double PLATFORM_LATENCY = 0.005;

  void deliverBuffer(int32_t frames, float sampleRate) {
    lastCallbackFrameCount_.store(frames);
    streamSampleRate_.store(sampleRate);
  }

  bool isIdle() const override {
    return false;
  }

 protected:
  [[nodiscard]] double platformInputLatency(double callbackSeconds) const override {
    return callbackSeconds + PLATFORM_LATENCY;
  }
};

} // namespace

TEST(AudioRecorderOutputStateTest, InputLatencyGivesThePlatformTheLastBuffersDuration) {
  LiveAudioRecorder recorder;
  EXPECT_DOUBLE_EQ(recorder.getInputLatency(), LiveAudioRecorder::PLATFORM_LATENCY);

  recorder.deliverBuffer(480, kSampleRate);

  EXPECT_DOUBLE_EQ(
      recorder.getInputLatency(), 480.0 / kSampleRate + LiveAudioRecorder::PLATFORM_LATENCY);
}

TEST(AudioRecorderOutputStateTest, AConnectedAdapterNodeIsFedAgainByTheNextSession) {
  AdapterFixture adapterFixture;
  IdleAudioRecorder recorder;
  recorder.connect(adapterFixture.handle);
  ASSERT_TRUE(recorder.prepareOutputsLocked(streamFormat(2)).is_ok());
  ASSERT_TRUE(recorder.finishOutputs().is_ok());
  EXPECT_FALSE(recorder.isConnected());
  EXPECT_TRUE(adapterFixture.adapter().buff_.empty());

  ASSERT_TRUE(recorder.prepareOutputsLocked(streamFormat(1)).is_ok());

  EXPECT_TRUE(recorder.isConnected());
  EXPECT_EQ(adapterFixture.adapter().buff_.size(), 1U);
}

TEST(AudioRecorderOutputStateTest, DestroyingARecorderClosesTheFileItLeftOpen) {
  FakeEncoderLog log;
  const auto backend = makeFakeBackend(log);
  {
    IdleAudioRecorder recorder(&backend);
    ASSERT_TRUE(recorder.enableFileOutput(wavProperties()).is_ok());
    ASSERT_TRUE(recorder.prepareOutputsLocked(streamFormat(1)).is_ok());
    ASSERT_TRUE(recorder.usesFileOutput());
  }

  EXPECT_EQ(log.closedFiles.load(), 1);
}

// NOLINTEND
