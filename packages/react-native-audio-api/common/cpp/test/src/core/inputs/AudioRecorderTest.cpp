#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/inputs/IRecorderInput.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
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
    recorder_ = std::make_shared<AudioRecorder>(registry_, input_, &backend_);
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

  /// Connects an adapter node to the recorder and returns it; the handle keeps it alive.
  RecorderAdapterNode *connectAdapter() {
    adapterContext_ = std::make_shared<OfflineAudioContext>(
        2, 512, 48000.0F, std::make_shared<MockAudioEventHandlerRegistry>());
    auto adapter = std::make_unique<RecorderAdapterNode>(adapterContext_);
    auto *adapterNode = adapter.get();
    adapterHandle_ = std::make_shared<utils::graph::NodeHandle>(std::move(adapter));
    recorder_->connect(adapterHandle_);
    return adapterNode;
  }

  /// Expects exactly @p count buffers to reach the JS callback. Exact only once the session is
  /// stopped, which joins the callback's worker thread and flushes what it still holds.
  void expectAudioReadyEvents(int count) {
    EXPECT_CALL(*registry_, dispatchEvent(AudioEvent::AUDIO_READY, CALLBACK_ID, testing::_))
        .Times(count);
  }

  static constexpr int FRAMES = 128;
  static constexpr uint64_t CALLBACK_ID = 99;

  std::shared_ptr<testing::NiceMock<MockAudioEventHandlerRegistry>> registry_ =
      std::make_shared<testing::NiceMock<MockAudioEventHandlerRegistry>>();
  std::shared_ptr<OfflineAudioContext> adapterContext_;
  std::shared_ptr<utils::graph::NodeHandle> adapterHandle_;

  FakeEncoderLog log_;
  PlatformFileBackend backend_;
  std::shared_ptr<FakeRecorderInput> input_ = std::make_shared<FakeRecorderInput>();
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

TEST_F(AudioRecorderTest, FramesInALayoutTheOutputsDoNotExpectAreDropped) {
  enableFileOutput();
  ASSERT_TRUE(recorder_->start().is_ok());

  input_->deliverFrames(FRAMES, /*channelCount=*/2);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_EQ(encodedBuffers(), 0);
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
}

TEST_F(AudioRecorderTest, AnInputThatFailsToOpenLeavesTheRecorderIdle) {
  input_->openError = "no microphone";

  auto result = recorder_->start();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "no microphone");
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_EQ(input_->routesReleased.load(), 1);
}

TEST_F(AudioRecorderTest, AnInputThatFailsToStartIsClosedWithTheFileItOpened) {
  enableFileOutput();
  input_->startError = "stream refused to start";

  ASSERT_TRUE(recorder_->start().is_err());
  EXPECT_TRUE(recorder_->isIdle());
  EXPECT_FALSE(input_->isOpen);
  EXPECT_EQ(log_.closedFiles.load(), 1);

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

TEST_F(AudioRecorderTest, AnInputFormatChangeMidRecordingRepreparesTheOutputs) {
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
  ASSERT_TRUE(recorder_->setOnAudioReadyCallback(48000.0F, FRAMES, 1, CALLBACK_ID).is_ok());
  EXPECT_FALSE(recorder_->usesCallback());

  ASSERT_TRUE(recorder_->start().is_ok());
  EXPECT_TRUE(recorder_->usesCallback());
  input_->deliverFrames(FRAMES);
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  EXPECT_FALSE(recorder_->usesCallback());
}

TEST_F(AudioRecorderTest, ACallbackIsFedAgainByTheNextSession) {
  expectAudioReadyEvents(2);
  ASSERT_TRUE(recorder_->setOnAudioReadyCallback(48000.0F, FRAMES, 1, CALLBACK_ID).is_ok());

  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());

  ASSERT_TRUE(recorder_->start().is_ok());
  EXPECT_TRUE(recorder_->usesCallback());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, ACallbackSetWhileRecordingIsFedAtOnce) {
  expectAudioReadyEvents(1);
  ASSERT_TRUE(recorder_->start().is_ok());

  ASSERT_TRUE(recorder_->setOnAudioReadyCallback(48000.0F, FRAMES, 1, CALLBACK_ID).is_ok());
  EXPECT_TRUE(recorder_->usesCallback());
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, ACallbackThatCannotBePreparedFailsTheStart) {
  ASSERT_TRUE(
      recorder_->setOnAudioReadyCallback(/*sampleRate=*/0.0F, FRAMES, 1, CALLBACK_ID).is_ok());

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
  ASSERT_TRUE(recorder_->setOnAudioReadyCallback(48000.0F, FRAMES, 1, CALLBACK_ID).is_ok());
  ASSERT_TRUE(recorder_->start().is_ok());
  input_->deliverFrames(FRAMES / 2);

  input_->format.layout.channelCount = 2;
  input_->events->formatChanged();
  input_->deliverFrames(FRAMES);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, AnOutputThatCannotFollowAFormatChangeDoesNotHoldTheOthersBack) {
  input_->format.layout.channelCount = 2;
  enableFileOutput();
  auto *adapter = connectAdapter();
  ASSERT_TRUE(recorder_->start().is_ok());
  ASSERT_EQ(adapter->buff_.size(), 2U);
  log_.failNextReprepare.store(true);

  input_->format.layout.channelCount = 1;
  input_->events->formatChanged();

  EXPECT_FALSE(recorder_->usesFileOutput());
  EXPECT_TRUE(recorder_->isConnected());
  EXPECT_EQ(adapter->buff_.size(), 1U);
  input_->deliverFrames(FRAMES);
  EXPECT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, AConnectedAdapterNodeIsFedAgainByTheNextSession) {
  auto *adapter = connectAdapter();
  ASSERT_TRUE(recorder_->start().is_ok());
  ASSERT_TRUE(recorder_->stop().is_ok());
  EXPECT_FALSE(recorder_->isConnected());
  EXPECT_TRUE(adapter->buff_.empty());

  ASSERT_TRUE(recorder_->start().is_ok());

  EXPECT_TRUE(recorder_->isConnected());
  EXPECT_EQ(adapter->buff_.size(), 1U);
  ASSERT_TRUE(recorder_->stop().is_ok());
}

TEST_F(AudioRecorderTest, AFileThatFailsToCloseStillLeavesTheOtherOutputsCleanedUp) {
  enableFileOutput();
  auto *adapter = connectAdapter();
  ASSERT_TRUE(recorder_->start().is_ok());
  log_.failNextClose.store(true);

  auto stopResult = recorder_->stop();

  ASSERT_TRUE(stopResult.is_err());
  EXPECT_TRUE(adapter->buff_.empty());
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

// NOLINTEND
