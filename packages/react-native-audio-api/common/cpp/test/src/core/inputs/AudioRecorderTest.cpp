#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/inputs/RecordingSessionRegistry.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/AudioRecorderCallback.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <test/src/core/inputs/FakeRecorderInput.h>
#include <test/src/utils/FakeFileBackend.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace audioapi;
using namespace audioapi::test_support;

// NOLINTBEGIN

class AudioRecorderTest : public ::testing::Test {
 protected:
  void SetUp() override {
    backend_ = makeFakeBackend(log_);
    recorder_ = std::make_shared<AudioRecorder>(registry_, input_, sessions_, &backend_);
  }

  void TearDown() override {
    recorder_.reset();
  }

  void enableFileOutput() {
    ASSERT_TRUE(recorder_->enableFileOutput(wavFileProperties()).is_ok());
  }

  /// Buffers the file encoder received. Exact only once the file is closed, which joins its
  /// worker thread.
  int encodedBuffers() const {
    return log_.encodedBuffers.load();
  }

  /// Adds an adapter node's consumer to the recorder, as JS `AudioRecorder.connect` does, and
  /// returns it.
  RecorderAdapterConsumer *connectAdapter() {
    adapterContext_ = std::make_shared<OfflineAudioContext>(
        2, 512, 48000.0F, std::make_shared<MockAudioEventHandlerRegistry>());
    adapterNode_ = std::make_unique<RecorderAdapterNode>(adapterContext_);
    EXPECT_TRUE(recorder_->addConsumer(adapterNode_->consumer()).is_ok());
    return adapterNode_->consumer().get();
  }

  /// A JS callback delivering mono buffers of FRAMES frames at @p sampleRate.
  std::shared_ptr<AudioRecorderCallback> makeCallback(float sampleRate = 48000.0F) {
    return std::make_shared<AudioRecorderCallback>(registry_, sampleRate, FRAMES, 1, CALLBACK_ID);
  }

  /// Expects exactly @p count buffers to reach the JS callback. Exact only once the session is
  /// stopped, which joins the consumers' worker thread and flushes what the callback still holds.
  void expectAudioReadyEvents(int count) {
    EXPECT_CALL(*registry_, dispatchEvent(AudioEvent::AUDIO_READY, CALLBACK_ID, testing::_))
        .Times(count);
  }

  /// A second recorder reporting to the same registry, without consumers.
  std::shared_ptr<AudioRecorder> makeOtherRecorder() {
    return std::make_shared<AudioRecorder>(
        nullptr, std::make_shared<FakeRecorderInput>(), sessions_, &backend_);
  }

  /// False while a session of another recorder holds the registry's claim.
  bool anotherRecorderCanStart() {
    return makeOtherRecorder()->start().is_ok();
  }

  static constexpr int FRAMES = 128;
  static constexpr uint64_t CALLBACK_ID = 99;

  std::shared_ptr<testing::NiceMock<MockAudioEventHandlerRegistry>> registry_ =
      std::make_shared<testing::NiceMock<MockAudioEventHandlerRegistry>>();
  std::shared_ptr<OfflineAudioContext> adapterContext_;
  std::unique_ptr<RecorderAdapterNode> adapterNode_;

  FakeEncoderLog log_;
  PlatformFileBackend backend_;
  std::shared_ptr<FakeRecorderInput> input_ = std::make_shared<FakeRecorderInput>();
  std::shared_ptr<RecordingSessionRegistry> sessions_ =
      std::make_shared<RecordingSessionRegistry>();
  std::shared_ptr<AudioRecorder> recorder_;
};

TEST_F(AudioRecorderTest, StartOpensTheInputAndStopClosesItWithTheFile) {
  enableFileOutput();

  ASSERT_TRUE(recorder_->start().is_ok());
  EXPECT_TRUE(recorder_->isRecording());
  EXPECT_TRUE(input_->isOpen);
  EXPECT_EQ(input_->openPurposes, std::vector{InputOpenPurpose::StartSession});

  input_->deliverFrames(FRAMES);
  input_->deliverFrames(FRAMES);

  auto stopResult = recorder_->stop();
  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_EQ(stopResult.unwrap().paths.size(), 1U);
  EXPECT_EQ(encodedBuffers(), 2);
  EXPECT_EQ(log_.closedFiles.load(), 1);
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_FALSE(input_->isOpen);
  EXPECT_EQ(input_->routesReleased.load(), input_->routesAcquired.load());
}

TEST_F(AudioRecorderTest, FramesOutsideRecordingAreDropped) {
  enableFileOutput();

  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES);
  recorder_->pause();
  input_->deliverFrames(FRAMES);
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->resume().is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
  input_->deliverFrames(FRAMES);

  EXPECT_EQ(encodedBuffers(), 2);
}

TEST_F(AudioRecorderTest, FramesInALayoutTheConsumersDoNotExpectAreDropped) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->deliverFrames(FRAMES, /*channelCount=*/2);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(encodedBuffers(), 0);
}

TEST_F(AudioRecorderTest, ConsumersArePreparedForTheFormatTheStartedInputDelivers) {
  auto *adapter = connectAdapter();
  input_->formatAfterStart = StreamFormat{
      .layout = {.sampleRate = 48000.0F, .channelCount = 2},
      .maxFramesPerBuffer = 256,
  };

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_EQ(adapter->preparedChannelCount(), 2U);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, StartWhileRecordingIsRefused) {
  ASSERT_TRUE(recorder_->start().is_ok());

  auto second = recorder_->start();

  ASSERT_TRUE(second.is_err());
  EXPECT_EQ(second.unwrap_err(), "Recorder is already recording");
  EXPECT_EQ(input_->routesAcquired.load(), 1);
}

TEST_F(AudioRecorderTest, StopWhileIdleFails) {
  EXPECT_TRUE(recorder_->stop().is_err());
  EXPECT_FALSE(sessions_->consumeLastRecordingResult().has_value());
}

TEST_F(AudioRecorderTest, AStartWhileAnotherRecorderRecordsIsRefusedBeforeItsRouteIsAcquired) {
  auto other = makeOtherRecorder();
  ASSERT_TRUE(other->start().is_ok());

  auto result = recorder_->start();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "Another recording is already in progress");
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_TRUE(input_->openPurposes.empty());
  EXPECT_EQ(input_->routesAcquired.load(), 0);
  EXPECT_TRUE(other->isRecording());
  EXPECT_FALSE(anotherRecorderCanStart());
}

TEST_F(AudioRecorderTest, AStartWaitingForItsRouteHoldsTheSessionAndReadsAsRecording) {
  input_->duringAcquireRoute = [this] {
    EXPECT_TRUE(recorder_->isRecording());
    EXPECT_FALSE(anotherRecorderCanStart());
    EXPECT_TRUE(recorder_->start().is_err());
  };

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_TRUE(recorder_->isRecording());
  EXPECT_EQ(input_->routesAcquired.load(), 1);
  EXPECT_EQ(input_->openPurposes.size(), 1U);
}

// The race the claim decides: the start that comes second has not touched routing when it is
// refused, even though the first one is not recording yet.
TEST_F(AudioRecorderTest, AStartRacingAStartThatWaitsForItsRouteIsRefusedBeforeItsRouteIsAcquired) {
  auto otherInput = std::make_shared<FakeRecorderInput>();
  auto other = std::make_shared<AudioRecorder>(nullptr, otherInput, sessions_, &backend_);
  input_->duringAcquireRoute = [&other] {
    auto result = other->start();
    ASSERT_TRUE(result.is_err());
    EXPECT_EQ(result.unwrap_err(), "Another recording is already in progress");
  };

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_TRUE(recorder_->isRecording());
  EXPECT_TRUE(other->isIdle());
  EXPECT_EQ(otherInput->routesAcquired.load(), 0);
  EXPECT_TRUE(otherInput->openPurposes.empty());
}

TEST_F(AudioRecorderTest, AStopWhileAStartWaitsForItsRouteAbandonsTheStart) {
  enableFileOutput();
  input_->duringAcquireRoute = [this] {
    EXPECT_TRUE(recorder_->stop().is_ok());
  };

  auto result = recorder_->start();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "Recorder was stopped before it started");
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_TRUE(input_->openPurposes.empty());
  EXPECT_EQ(input_->routesReleased.load(), input_->routesAcquired.load());
  EXPECT_EQ(log_.closedFiles.load(), 0);
  EXPECT_FALSE(sessions_->consumeLastRecordingResult().has_value());
  EXPECT_TRUE(anotherRecorderCanStart());
}

// The session the second start began is not the one the first start claimed.
TEST_F(AudioRecorderTest, AStopAndStartWhileAStartWaitsForItsRouteLeaveTheNewSessionRecording) {
  input_->duringAcquireRoute = [this] {
    ASSERT_TRUE(recorder_->stop().is_ok());
    ASSERT_TRUE(recorder_->start().is_ok());
  };

  EXPECT_TRUE(recorder_->start().is_err());

  EXPECT_TRUE(recorder_->isRecording());
  EXPECT_EQ(input_->openPurposes.size(), 1U);
  EXPECT_EQ(input_->routesAcquired.load(), 2);
  EXPECT_EQ(input_->routesReleased.load(), 1);
}

TEST_F(AudioRecorderTest, APauseWhileAStartWaitsForItsRouteOpensTheInputStopped) {
  input_->duringAcquireRoute = [this] {
    recorder_->pause();
    EXPECT_TRUE(recorder_->isPaused());
    // The input opens stopped, so resuming has to wait for it.
    EXPECT_TRUE(recorder_->resume().is_err());
  };

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_TRUE(recorder_->isPaused());
  EXPECT_TRUE(input_->isOpen);
  EXPECT_FALSE(input_->running.load());
  EXPECT_TRUE(recorder_->resume().is_ok());
  EXPECT_TRUE(recorder_->isRecording());
}

// The route the start waits for was settled before the device changed: the start acquires again
// and opens only once, on the new device.
TEST_F(AudioRecorderTest, ARerouteWhileAStartWaitsForItsRouteOpensTheInputOnTheNewDevice) {
  enableFileOutput();
  static constexpr int32_t NEW_DEVICE = 7;
  input_->duringAcquireRoute = [this] {
    input_->routedDeviceId = NEW_DEVICE;
    EXPECT_TRUE(recorder_->rerouteInput().is_ok());
  };

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_TRUE(recorder_->isRecording());
  EXPECT_EQ(input_->openPurposes, std::vector<InputOpenPurpose>{InputOpenPurpose::StartSession});
  EXPECT_EQ(input_->openedDeviceIds, (std::vector<std::optional<int32_t>>{NEW_DEVICE}));
  EXPECT_EQ(input_->routesAcquired.load(), 2);
  EXPECT_EQ(input_->routesReleased.load(), 1);
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
  EXPECT_EQ(encodedBuffers(), 1);
  EXPECT_EQ(input_->routesReleased.load(), input_->routesAcquired.load());
}

TEST_F(AudioRecorderTest, ADeviceChangeDuringEveryRouteWaitOfAStartIsFollowedToTheLast) {
  static constexpr int32_t SECOND_DEVICE = 7;
  static constexpr int32_t THIRD_DEVICE = 9;
  input_->duringAcquireRoute = [this] {
    input_->routedDeviceId = SECOND_DEVICE;
    EXPECT_TRUE(recorder_->rerouteInput().is_ok());
    input_->duringAcquireRoute = [this] {
      input_->routedDeviceId = THIRD_DEVICE;
      EXPECT_TRUE(recorder_->rerouteInput().is_ok());
    };
  };

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_EQ(input_->openedDeviceIds, (std::vector<std::optional<int32_t>>{THIRD_DEVICE}));
  EXPECT_EQ(input_->routesAcquired.load(), 3);
  EXPECT_EQ(input_->routesReleased.load(), 2);
}

TEST_F(AudioRecorderTest, ARerouteWhileAStartWaitsKeepsASessionPausedMeanwhilePaused) {
  input_->duringAcquireRoute = [this] {
    EXPECT_TRUE(recorder_->rerouteInput().is_ok());
    recorder_->pause();
  };

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_TRUE(recorder_->isPaused());
  EXPECT_EQ(input_->openPurposes.size(), 1U);
  EXPECT_TRUE(input_->isOpen);
  EXPECT_FALSE(input_->running.load());
}

TEST_F(AudioRecorderTest, AStopWhileAStartAcquiresItsRouteAgainAbandonsTheStart) {
  input_->duringAcquireRoute = [this] {
    EXPECT_TRUE(recorder_->rerouteInput().is_ok());
    input_->duringAcquireRoute = [this] {
      EXPECT_TRUE(recorder_->stop().is_ok());
    };
  };

  EXPECT_TRUE(recorder_->start().is_err());

  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_TRUE(input_->openPurposes.empty());
  EXPECT_EQ(input_->routesAcquired.load(), 2);
  EXPECT_EQ(input_->routesReleased.load(), 2);
  EXPECT_TRUE(anotherRecorderCanStart());
}

TEST_F(AudioRecorderTest, AStartWhoseInputFailsToOpenReleasesTheSession) {
  input_->openError = "device busy";

  EXPECT_TRUE(recorder_->start().is_err());

  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_EQ(input_->routesReleased.load(), input_->routesAcquired.load());
  EXPECT_TRUE(anotherRecorderCanStart());
}

TEST_F(AudioRecorderTest, AStopReleasesTheSessionAndKeepsWhatItWrote) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  EXPECT_FALSE(anotherRecorderCanStart());

  auto stopResult = recorder_->stop();

  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_TRUE(anotherRecorderCanStart());
  auto kept = sessions_->consumeLastRecordingResult();
  ASSERT_TRUE(kept.has_value());
  EXPECT_EQ(kept->paths, stopResult.unwrap().paths);
}

/// Counts its prepares and closes; close() waits while holdClose is set, so a test can act while
/// a stop is closing it.
class HeldClosingConsumer : public AudioConsumer {
 public:
  Result<NoneType, std::string> prepare(const StreamFormat &) override {
    ++prepares;
    return Ok(None);
  }
  Result<NoneType, std::string> consume(const AudioBuffer &, size_t) override {
    return Ok(None);
  }
  void close() override {
    ++closes;
    isClosing = true;
    while (holdClose.load()) {
      std::this_thread::yield();
    }
  }

  std::atomic<bool> holdClose{false};
  std::atomic<bool> isClosing{false};
  std::atomic<int> prepares{0};
  std::atomic<int> closes{0};
};

TEST_F(AudioRecorderTest, AConsumerSharedWithAnotherRecorderIsClosedBeforeTheSessionIsReleased) {
  auto shared = std::make_shared<HeldClosingConsumer>();
  auto other = makeOtherRecorder();
  ASSERT_TRUE(recorder_->addConsumer(shared).is_ok());
  ASSERT_TRUE(other->addConsumer(shared).is_ok());
  ASSERT_TRUE(recorder_->start().is_ok());

  shared->holdClose = true;
  std::thread stopping([this] { EXPECT_TRUE(recorder_->stop().is_ok()); });
  while (!shared->isClosing.load()) {
    std::this_thread::yield();
  }

  // Still closing the shared consumer: the next session must not prepare it yet.
  EXPECT_TRUE(other->start().is_err());
  EXPECT_EQ(shared->prepares.load(), 1);

  shared->holdClose = false;
  stopping.join();
  ASSERT_TRUE(other->start().is_ok());
  EXPECT_EQ(shared->prepares.load(), 2);
  EXPECT_EQ(shared->closes.load(), 1);
  EXPECT_TRUE(other->stop().is_ok());
}

TEST_F(AudioRecorderTest, AnInputThatFailsToOpenLeavesTheRecorderIdle) {
  input_->openError = "no microphone";

  auto result = recorder_->start();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "no microphone");
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_EQ(input_->routesReleased.load(), 1);
  EXPECT_TRUE(anotherRecorderCanStart());
}

TEST_F(AudioRecorderTest, AnInputThatFailsToStartIsClosedBeforeAnyFileIsOpened) {
  enableFileOutput();
  input_->startError = "stream refused to start";

  ASSERT_TRUE(recorder_->start().is_err());
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_FALSE(input_->isOpen);
  EXPECT_EQ(log_.closedFiles.load(), 0);
  EXPECT_TRUE(anotherRecorderCanStart());
  EXPECT_FALSE(sessions_->consumeLastRecordingResult().has_value());

  input_->startError.reset();
  EXPECT_TRUE(recorder_->start().is_ok());
}

TEST_F(AudioRecorderTest, ResumeFollowsAFormatThatChangedWhilePaused) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  recorder_->pause();

  input_->format.layout.channelCount = 2;
  ASSERT_TRUE(recorder_->resume().is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(log_.lastRepreparedFormat.layout.channelCount, 2);
  EXPECT_EQ(encodedBuffers(), 1);
}

TEST_F(AudioRecorderTest, RefusedResumeKeepsTheSessionPaused) {
  ASSERT_TRUE(recorder_->start().is_ok());
  recorder_->pause();
  input_->resumeError = "engine refused to start";

  auto result = recorder_->resume();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "engine refused to start");
  EXPECT_TRUE(recorder_->isPaused());
}

TEST_F(AudioRecorderTest, ARefusedResumeOfASessionTheSystemPausedKeepsItsFramesFlowing) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->running.store(false);
  input_->resumeError = "session busy";

  ASSERT_TRUE(recorder_->resume().is_err());

  // The system restarts capture on its own, with no notification.
  input_->running.store(true);
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(encodedBuffers(), 1);
}

TEST_F(AudioRecorderTest, ARefusedResumeStillFollowsAFormatTheInputChangedTo) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->running.store(false);
  // Same channel count, so only following the format keeps these frames out of the old one.
  input_->format.layout.sampleRate = 16000.0F;
  input_->resumeError = "session busy";

  ASSERT_TRUE(recorder_->resume().is_err());

  // The system restarts capture on its own, with no notification.
  input_->running.store(true);
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_FLOAT_EQ(log_.lastRepreparedFormat.layout.sampleRate, 16000.0F);
  EXPECT_EQ(encodedBuffers(), 1);
}

TEST_F(AudioRecorderTest, AResumeWhoseFormatCannotBeReadIsUndone) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  recorder_->pause();
  input_->formatUnavailable = true;

  auto result = recorder_->resume();

  ASSERT_TRUE(result.is_err());
  EXPECT_TRUE(recorder_->isPaused());
  EXPECT_FALSE(input_->running.load());

  input_->formatUnavailable = false;
  ASSERT_TRUE(recorder_->resume().is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
  EXPECT_EQ(encodedBuffers(), 1);
}

TEST_F(AudioRecorderTest, AnInputFormatChangeMidRecordingRepreparesTheConsumers) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->format.layout.channelCount = 2;
  input_->events->formatChanged();
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(log_.lastRepreparedFormat.layout.channelCount, 2);
  EXPECT_EQ(log_.openedFileNames.size(), 1U);
  EXPECT_EQ(encodedBuffers(), 1);
}

TEST_F(AudioRecorderTest, AnUnavailableInputFormatLeavesTheSessionAsItIs) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->formatUnavailable = true;
  input_->events->formatChanged();
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(encodedBuffers(), 1);
}

TEST_F(AudioRecorderTest, AMonoCallbackIsFedFromAMultichannelInput) {
  input_->format.layout.channelCount = 2;
  expectAudioReadyEvents(2);
  ASSERT_TRUE(recorder_->addConsumer(makeCallback()).is_ok());

  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES);
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, ACallbackIsFedAgainByTheNextSession) {
  expectAudioReadyEvents(2);
  ASSERT_TRUE(recorder_->addConsumer(makeCallback()).is_ok());

  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, AClearedCallbackIsNotFed) {
  expectAudioReadyEvents(0);
  auto callback = makeCallback();
  ASSERT_TRUE(recorder_->addConsumer(callback).is_ok());
  recorder_->removeConsumer(callback);

  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, AnInvalidatedCallbackReleasesItsHandlerOnceAndIsNoLongerDelivered) {
  expectAudioReadyEvents(0);
  EXPECT_CALL(*registry_, unregisterHandler(AudioEvent::AUDIO_READY, CALLBACK_ID)).Times(1);
  auto callback = makeCallback();
  ASSERT_TRUE(recorder_->addConsumer(callback).is_ok());
  ASSERT_TRUE(recorder_->start().is_ok());

  callback->invalidate();
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
  recorder_->removeConsumer(callback);
  callback.reset();
}

TEST_F(AudioRecorderTest, ACallbackSetWhileRecordingIsFedAtOnce) {
  expectAudioReadyEvents(1);
  ASSERT_TRUE(recorder_->start().is_ok());

  ASSERT_TRUE(recorder_->addConsumer(makeCallback()).is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, ACallbackThatCannotBePreparedFailsTheStart) {
  ASSERT_TRUE(recorder_->addConsumer(makeCallback(/*sampleRate=*/0.0F)).is_ok());

  auto result = recorder_->start();

  ASSERT_TRUE(result.is_err());
  EXPECT_NE(result.unwrap_err().find("Failed to prepare callback"), std::string::npos);
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_FALSE(input_->isOpen);
  EXPECT_EQ(input_->routesReleased.load(), input_->routesAcquired.load());
}

TEST_F(AudioRecorderTest, AnInputFormatChangeFlushesTheCallbackBeforeRepreparingIt) {
  // Half a callback buffer in the old format, emitted on its own, then a full one in the new.
  expectAudioReadyEvents(2);
  ASSERT_TRUE(recorder_->addConsumer(makeCallback()).is_ok());
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES / 2);

  input_->format.layout.channelCount = 2;
  input_->events->formatChanged();
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, AConsumerThatCannotFollowAFormatChangeDoesNotHoldTheOthersBack) {
  input_->format.layout.channelCount = 2;
  enableFileOutput();
  auto *adapter = connectAdapter();
  ASSERT_TRUE(recorder_->start().is_ok());
  ASSERT_EQ(adapter->preparedChannelCount(), 2U);
  log_.failNextReprepare.store(true);

  input_->format.layout.channelCount = 1;
  input_->events->formatChanged();

  EXPECT_EQ(adapter->preparedChannelCount(), 1U);
  input_->deliverFrames(FRAMES);
  EXPECT_TRUE(recorder_->stop().is_ok());
  EXPECT_EQ(encodedBuffers(), 0);
}

TEST_F(AudioRecorderTest, AFileThatCannotFollowAFormatChangeIsStillReturned) {
  input_->format.layout.channelCount = 2;
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  log_.failNextReprepare.store(true);

  input_->format.layout.channelCount = 1;
  input_->events->formatChanged();
  auto stopResult = recorder_->stop();

  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_EQ(stopResult.unwrap().paths.size(), 1U);
  EXPECT_EQ(log_.closedFiles.load(), 1);
}

TEST_F(AudioRecorderTest, AnAdapterWhoseContextIsGoneDoesNotStopTheRecording) {
  enableFileOutput();
  auto *adapter = connectAdapter();
  adapterContext_.reset();

  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES);
  auto stopResult = recorder_->stop();

  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_EQ(stopResult.unwrap().paths.size(), 1U);
  EXPECT_EQ(adapter->preparedChannelCount(), 0U);
}

TEST_F(AudioRecorderTest, AConnectedAdapterNodeIsFedAgainByTheNextSession) {
  auto *adapter = connectAdapter();
  ASSERT_TRUE(recorder_->start().is_ok());
  ASSERT_TRUE(recorder_->stop().is_ok());
  EXPECT_EQ(adapter->preparedChannelCount(), 0U);

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_EQ(adapter->preparedChannelCount(), 1U);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, AFileThatFailsToCloseStillLeavesTheOtherConsumersCleanedUp) {
  enableFileOutput();
  auto *adapter = connectAdapter();
  ASSERT_TRUE(recorder_->start().is_ok());
  log_.failNextClose.store(true);

  auto stopResult = recorder_->stop();

  ASSERT_TRUE(stopResult.is_err());
  EXPECT_EQ(adapter->preparedChannelCount(), 0U);
  EXPECT_TRUE(recorder_->isIdle());
}

TEST_F(AudioRecorderTest, RerouteReopensTheInputAndKeepsRecordingIntoTheSameFile) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  ASSERT_TRUE(recorder_->rerouteInput().is_ok());
  EXPECT_EQ(
      input_->openPurposes,
      (std::vector{InputOpenPurpose::StartSession, InputOpenPurpose::ContinueSession}));
  EXPECT_TRUE(recorder_->isRecording());

  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(encodedBuffers(), 1);
  EXPECT_EQ(log_.openedFileNames.size(), 1U);
  EXPECT_EQ(input_->routesAcquired.load(), 2);
  EXPECT_EQ(input_->routesReleased.load(), 2);
}

TEST_F(AudioRecorderTest, RerouteOfAPausedSessionLeavesItPaused) {
  ASSERT_TRUE(recorder_->start().is_ok());
  recorder_->pause();

  ASSERT_TRUE(recorder_->rerouteInput().is_ok());

  EXPECT_TRUE(recorder_->isPaused());
  EXPECT_TRUE(input_->isOpen);
  EXPECT_FALSE(input_->running.load());
}

TEST_F(AudioRecorderTest, RerouteWhileIdleDoesNothing) {
  ASSERT_TRUE(recorder_->rerouteInput().is_ok());
  EXPECT_EQ(input_->routesAcquired.load(), 0);
}

TEST_F(AudioRecorderTest, AStopWhileARerouteWaitsForItsRouteAbandonsTheReroute) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->duringAcquireRoute = [this] {
    ASSERT_TRUE(recorder_->stop().is_ok());
  };

  ASSERT_TRUE(recorder_->rerouteInput().is_ok());

  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_FALSE(input_->isOpen);
  EXPECT_EQ(input_->openPurposes.size(), 1U);
  EXPECT_EQ(input_->routesReleased.load(), input_->routesAcquired.load());
  EXPECT_EQ(log_.closedFiles.load(), 1);
}

TEST_F(AudioRecorderTest, AnInputThatCannotBeReopenedEndsTheSessionAndClosesItsFile) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->openError = "device gone";

  auto result = recorder_->rerouteInput();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "device gone");
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_FALSE(input_->isOpen);
  EXPECT_EQ(input_->routesReleased.load(), input_->routesAcquired.load());
  EXPECT_EQ(log_.closedFiles.load(), 1);
}

TEST_F(AudioRecorderTest, PauseAndResumeWhileAReroutedInputIsOnItsWayAreRemembered) {
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->duringAcquireRoute = [this] {
    recorder_->pause();
    EXPECT_TRUE(recorder_->isPaused());
    // The reopened input comes back stopped, so resuming has to wait for it.
    EXPECT_TRUE(recorder_->resume().is_err());
  };

  ASSERT_TRUE(recorder_->rerouteInput().is_ok());

  EXPECT_TRUE(recorder_->isPaused());
  EXPECT_TRUE(input_->isOpen);
  EXPECT_FALSE(input_->running.load());
  EXPECT_TRUE(recorder_->resume().is_ok());
  EXPECT_TRUE(recorder_->isRecording());
}

TEST_F(AudioRecorderTest, ResumeWhileIdleFails) {
  auto result = recorder_->resume();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "Recorder is not in recording state.");
}

TEST_F(AudioRecorderTest, ASessionWhoseInputIsNotRunningReadsAsPaused) {
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->running.store(false);

  EXPECT_FALSE(recorder_->isRecording());
  EXPECT_TRUE(recorder_->isPaused());
  EXPECT_EQ(recorder_->getState(), RecorderState::Paused);
}

TEST_F(AudioRecorderTest, DestroyingARecordingRecorderFinalizesItsFile) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  recorder_.reset();

  EXPECT_EQ(log_.closedFiles.load(), 1);
  EXPECT_TRUE(anotherRecorderCanStart());
  auto kept = sessions_->consumeLastRecordingResult();
  ASSERT_TRUE(kept.has_value());
  EXPECT_EQ(kept->paths.size(), 1U);
}

TEST_F(AudioRecorderTest, EventsFromAnEarlierOpeningOfTheInputAreIgnored) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  const auto staleEvents = *input_->events;
  ASSERT_TRUE(recorder_->stop().is_ok());
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->format.layout.channelCount = 2;
  staleEvents.formatChanged();
  staleEvents.lost();
  input_->deliverFrames(FRAMES, /*channelCount=*/1);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(log_.lastRepreparedFormat.layout.channelCount, 0);
  EXPECT_EQ(input_->openPurposes.size(), 2U);
  EXPECT_EQ(encodedBuffers(), 1);
}

TEST_F(AudioRecorderTest, ALostInputIsReopened) {
  ASSERT_TRUE(recorder_->start().is_ok());

  const auto firstOpening = *input_->events;
  firstOpening.lost();
  // The reroute closed that opening, so a second report of the same loss is dropped.
  firstOpening.lost();

  EXPECT_EQ(
      input_->openPurposes,
      (std::vector{InputOpenPurpose::StartSession, InputOpenPurpose::ContinueSession}));
  EXPECT_TRUE(recorder_->isRecording());
}

TEST_F(AudioRecorderTest, ALostInputThatCannotBeReopenedEndsTheSession) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->openError = "device gone";

  input_->events->lost();

  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_EQ(log_.closedFiles.load(), 1);
}

TEST_F(AudioRecorderTest, ARecordingEndedByAFailedRerouteIsKept) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->openError = "device gone";

  EXPECT_TRUE(recorder_->rerouteInput().is_err());

  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_TRUE(anotherRecorderCanStart());
  auto kept = sessions_->consumeLastRecordingResult();
  ASSERT_TRUE(kept.has_value());
  EXPECT_EQ(kept->paths.size(), 1U);
}

// The cleanup of a failed reroute runs after the control mutex is released, so a later reroute
// can reopen the input first; that cleanup must not end the session the later reroute restored.
TEST_F(AudioRecorderTest, AStopBoundToAnOpeningARerouteMovedPastLeavesTheSessionRecording) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());
  // start() opens the input at the recorder's first generation.
  constexpr uint64_t FIRST_OPENING = 0;
  ASSERT_TRUE(recorder_->rerouteInput().is_ok());

  EXPECT_TRUE(recorder_->stop(FIRST_OPENING).is_err());

  EXPECT_TRUE(recorder_->isRecording());
  EXPECT_FALSE(anotherRecorderCanStart());
  EXPECT_EQ(log_.closedFiles.load(), 0);
  EXPECT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, CurrentDurationCountsRecordedInputWithoutAnyConsumer) {
  auto clock = recorder_->getDurationClock();
  EXPECT_DOUBLE_EQ(clock->load(), 0.0);
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->deliverFrames(480);
  EXPECT_DOUBLE_EQ(clock->load(), 480.0 / 48000.0);

  recorder_->pause();
  input_->deliverFrames(480);
  EXPECT_DOUBLE_EQ(clock->load(), 480.0 / 48000.0);
  ASSERT_TRUE(recorder_->resume().is_ok());

  input_->format.layout.sampleRate = 24000.0F;
  input_->events->formatChanged();
  input_->deliverFrames(480);
  EXPECT_DOUBLE_EQ(clock->load(), 480.0 / 48000.0 + 480.0 / 24000.0);

  ASSERT_TRUE(recorder_->stop().is_ok());
  EXPECT_DOUBLE_EQ(clock->load(), 0.0);
}

TEST_F(AudioRecorderTest, TheDurationClockFollowsTheRecordingAndOutlivesTheRecorder) {
  auto clock = recorder_->getDurationClock();
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->deliverFrames(FRAMES);
  EXPECT_DOUBLE_EQ(clock->load(), FRAMES / 48000.0);

  std::weak_ptr<AudioRecorder> destroyed = recorder_;
  recorder_.reset();
  EXPECT_TRUE(destroyed.expired());
  EXPECT_DOUBLE_EQ(clock->load(), 0.0);
}

TEST_F(AudioRecorderTest, InputLatencyIsZeroWhileIdleAndUsesTheLastBufferOtherwise) {
  EXPECT_DOUBLE_EQ(recorder_->getInputLatency(), 0.0);

  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(480);

  EXPECT_DOUBLE_EQ(recorder_->getInputLatency(), 480.0 / 48000.0);
}

// The audio thread delivers continuously while the control thread disarms and re-arms the gate;
// run under ThreadSanitizer to check the hand-off.
TEST_F(AudioRecorderTest, DeliveringWhilePausingAndResumingIsRaceFree) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  std::atomic<bool> keepDelivering{true};
  std::thread audioThread([&] {
    while (keepDelivering.load()) {
      input_->deliverFrames(FRAMES);
    }
  });

  for (int iteration = 0; iteration < 200; ++iteration) {
    recorder_->pause();
    ASSERT_TRUE(recorder_->resume().is_ok());
  }

  keepDelivering.store(false);
  audioThread.join();
  EXPECT_TRUE(recorder_->stop().is_ok());
}

// The gate holds a raw pointer to the recorder and outlives it, and the audio thread keeps
// delivering through it while the recorder is destroyed and afterwards; run under
// AddressSanitizer and ThreadSanitizer to check that no callback reaches the destroyed recorder.
TEST_F(AudioRecorderTest, DestroyingARecorderWhileItsInputDeliversIsSafe) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  std::atomic<int> callbacks{0};
  std::atomic<bool> keepDelivering{true};
  std::thread audioThread([&] {
    while (keepDelivering.load()) {
      input_->deliverFrames(FRAMES);
      callbacks.fetch_add(1);
    }
  });

  while (callbacks.load() < 100) {
    std::this_thread::yield();
  }
  recorder_.reset();

  const int callbacksAtDestruction = callbacks.load();
  while (callbacks.load() < callbacksAtDestruction + 100) {
    std::this_thread::yield();
  }
  keepDelivering.store(false);
  audioThread.join();

  EXPECT_EQ(log_.closedFiles.load(), 1);
}

// NOLINTEND
