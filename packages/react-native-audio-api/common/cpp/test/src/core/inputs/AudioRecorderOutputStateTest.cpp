#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioFileProperties.h>
#include <gtest/gtest.h>
#include <test/src/core/inputs/FakeRecorderInput.h>
#include <test/src/utils/FakeFileBackend.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

using namespace audioapi;
using namespace audioapi::test_support;

// NOLINTBEGIN

namespace audioapi {

/// Reaches the recorder's output bookkeeping, which is private.
struct AudioRecorderTestPeer {
  using OutputState = AudioRecorder::OutputState;
  using FileSlot = AudioRecorder::OutputSlot<AudioFileWriter>;

  static bool wantsCallback(const AudioRecorder &recorder) {
    return recorder.callback_.isWanted();
  }

  static bool wantsFileOutput(const AudioRecorder &recorder) {
    return recorder.file_.isWanted();
  }

  static bool wantsConnection(const AudioRecorder &recorder) {
    return recorder.adapter_.isWanted();
  }

  static Result<NoneType, std::string> prepareOutputs(
      AudioRecorder &recorder,
      const StreamFormat &format) {
    std::scoped_lock outputsLock(
        recorder.callback_.mutex, recorder.file_.mutex, recorder.adapter_.mutex);
    return recorder.prepareOutputs(format);
  }
};

} // namespace audioapi

namespace {

using OutputState = AudioRecorderTestPeer::OutputState;

/// Never started, so every output it is given stays at Requested at most.
std::shared_ptr<AudioRecorder> createIdleRecorder() {
  return std::make_shared<AudioRecorder>(nullptr, std::make_shared<FakeRecorderInput>());
}

} // namespace

TEST(AudioRecorderOutputStateTest, DeactivateDemotesAnActiveOutputToRequested) {
  AudioRecorderTestPeer::FileSlot slot;
  slot.set(OutputState::Active);

  slot.deactivate();

  EXPECT_FALSE(slot.isActive());
  EXPECT_TRUE(slot.isWanted());
}

TEST(AudioRecorderOutputStateTest, DeactivateNeverEnablesADisabledOutput) {
  AudioRecorderTestPeer::FileSlot slot;

  slot.deactivate();

  EXPECT_FALSE(slot.isWanted());
}

TEST(AudioRecorderOutputStateTest, InvalidFilePropertiesAreRejectedAtEnable) {
  auto recorder = createIdleRecorder();
  auto properties = wavFileProperties();
  properties->fileLayout.sampleRate = 0.0F;

  EXPECT_TRUE(recorder->enableFileOutput(properties).is_err());
  EXPECT_FALSE(AudioRecorderTestPeer::wantsFileOutput(*recorder));
}

TEST(AudioRecorderOutputStateTest, AFormatThePlatformCannotWriteIsRejectedAtEnable) {
  auto recorder = createIdleRecorder();
  auto properties = wavFileProperties();
  properties->encoding.format = AudioFileProperties::FileFormat::OPUS_WEBM;

  EXPECT_TRUE(recorder->enableFileOutput(properties).is_err());
  EXPECT_FALSE(AudioRecorderTestPeer::wantsFileOutput(*recorder));
}

TEST(AudioRecorderOutputStateTest, CallbackIsRequestedUntilAStreamPreparesIt) {
  auto recorder = createIdleRecorder();

  ASSERT_TRUE(recorder->setOnAudioReadyCallback(48000.0F, 1024, 1, 1).is_ok());
  EXPECT_TRUE(AudioRecorderTestPeer::wantsCallback(*recorder));
  EXPECT_FALSE(recorder->usesCallback());

  recorder->clearOnAudioReadyCallback();
  EXPECT_FALSE(AudioRecorderTestPeer::wantsCallback(*recorder));
}

TEST(AudioRecorderOutputStateTest, AConnectionWhoseAdapterNodeIsGoneFailsToPrepare) {
  auto recorder = createIdleRecorder();
  recorder->connect(std::make_shared<utils::graph::NodeHandle>(nullptr));
  ASSERT_TRUE(AudioRecorderTestPeer::wantsConnection(*recorder));

  auto result = AudioRecorderTestPeer::prepareOutputs(
      *recorder,
      StreamFormat{
          .layout = AudioLayout{.sampleRate = 48000.0F, .channelCount = 1},
          .maxFramesPerBuffer = 1024,
      });

  EXPECT_TRUE(result.is_err());
  EXPECT_FALSE(recorder->isConnected());
  EXPECT_TRUE(AudioRecorderTestPeer::wantsConnection(*recorder));
}

// NOLINTEND
