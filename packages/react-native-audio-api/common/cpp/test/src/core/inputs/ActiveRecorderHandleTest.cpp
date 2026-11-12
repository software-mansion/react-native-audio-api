#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/utils/AudioFileWriter.h>
#include <gtest/gtest.h>
#include <test/src/core/inputs/FakeRecorderInput.h>
#include <test/src/utils/FakeFileBackend.h>

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

using namespace audioapi;
using namespace audioapi::test_support;

// NOLINTBEGIN

namespace audioapi {

/// Grants the tests the private constructor, so each case can act on a handle of its
/// own instead of the process-global one they would otherwise share.
struct ActiveRecorderHandleTestPeer {
  static std::unique_ptr<ActiveRecorderHandle> createHandle() {
    return std::unique_ptr<ActiveRecorderHandle>(new ActiveRecorderHandle());
  }
};

} // namespace audioapi

namespace {

/// A recorder over a fake input that takes its session from @p handle, writing its file through
/// a fake encoder whose log counts the files it closed. Declare it after the handle it uses.
struct FakeSession {
  explicit FakeSession(ActiveRecorderHandle &handle, bool writesFile = true)
      : backend(makeFakeBackend(log)) {
    recorder = std::make_shared<AudioRecorder>(nullptr, input, &backend, &handle);
    if (writesFile) {
      EXPECT_TRUE(recorder->enableFileOutput(wavFileProperties()).is_ok());
    }
  }

  FakeEncoderLog log;
  PlatformFileBackend backend;
  std::shared_ptr<FakeRecorderInput> input = std::make_shared<FakeRecorderInput>();
  // Last, so it is destroyed before the backend it writes through.
  std::shared_ptr<AudioRecorder> recorder;
};

} // namespace

TEST(ActiveRecorderHandleTest, WithoutASessionThereIsNothingToReportOrStop) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;

  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_EQ(handle.pause(), RecorderState::Idle);
  EXPECT_EQ(handle.resume(), RecorderState::Idle);
  EXPECT_TRUE(handle.rerouteInput().is_ok());
  EXPECT_TRUE(handle.stopAndReturnInfo().is_err());
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, AStoppedSessionIsNoLongerOngoing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());
  ASSERT_TRUE(handle.stopAndReturnInfo().is_ok());

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_TRUE(handle.stopAndReturnInfo().is_err());
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, RecordingAndPausedCountAsOngoing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());

  EXPECT_EQ(handle.currentState(), RecorderState::Recording);
  EXPECT_TRUE(handle.isRecordingOngoing());

  session.recorder->pause();
  EXPECT_EQ(handle.currentState(), RecorderState::Paused);
  EXPECT_TRUE(handle.isRecordingOngoing());
}

TEST(ActiveRecorderHandleTest, PauseAndResumeReachTheRecorderThatHoldsTheSession) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());

  EXPECT_EQ(handle.resume(), RecorderState::Recording);
  EXPECT_EQ(handle.pause(), RecorderState::Paused);
  EXPECT_TRUE(session.recorder->isPaused());

  EXPECT_EQ(handle.pause(), RecorderState::Paused);
  EXPECT_EQ(handle.resume(), RecorderState::Recording);
  EXPECT_TRUE(session.recorder->isRecording());
}

TEST(ActiveRecorderHandleTest, ARefusedResumeLeavesTheSessionPaused) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());
  handle.pause();
  session.input->resumeError = "session activation refused";

  EXPECT_EQ(handle.resume(), RecorderState::Paused);
  EXPECT_TRUE(session.recorder->isPaused());
}

TEST(ActiveRecorderHandleTest, AStoppedRecordingIsKeptForSingleConsumption) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());

  auto stopResult = handle.stopAndReturnInfo();
  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_FALSE(handle.isRecordingOngoing());

  auto result = handle.consumeLastRecordingResult();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->paths.size(), 1U);
  EXPECT_EQ(result->paths, stopResult.unwrap().paths);
  EXPECT_DOUBLE_EQ(result->size, session.log.fileSizeMB);
  EXPECT_DOUBLE_EQ(result->duration, session.log.fileDurationSec);

  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, ARecordingStoppedThroughItsRecorderIsKeptToo) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());

  auto stopResult = session.recorder->stop();
  ASSERT_TRUE(stopResult.is_ok());

  EXPECT_FALSE(handle.isRecordingOngoing());
  auto result = handle.consumeLastRecordingResult();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->paths, stopResult.unwrap().paths);
}

TEST(ActiveRecorderHandleTest, ASessionWithoutFileOutputLeavesNothingToConsume) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle, /*writesFile=*/false);
  ASSERT_TRUE(session.recorder->start().is_ok());

  ASSERT_TRUE(handle.stopAndReturnInfo().is_ok());
  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, ARecorderThatIsGoneHoldsNoSession) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  {
    FakeSession session(handle);
    ASSERT_TRUE(session.recorder->start().is_ok());
  }

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_EQ(handle.pause(), RecorderState::Idle);
  EXPECT_EQ(handle.resume(), RecorderState::Idle);
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, ASecondRecorderCannotStartWhileASessionIsInProgress) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession current(handle);
  FakeSession other(handle);
  ASSERT_TRUE(current.recorder->start().is_ok());

  auto otherResult = other.recorder->start();
  ASSERT_TRUE(otherResult.is_err());
  EXPECT_EQ(otherResult.unwrap_err(), "Another recording is already in progress");
  EXPECT_EQ(other.input->routesAcquired.load(), 0);
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);

  current.recorder->pause();
  EXPECT_TRUE(other.recorder->start().is_err());
  EXPECT_EQ(handle.currentState(), RecorderState::Paused);

  ASSERT_TRUE(current.recorder->stop().is_ok());
  ASSERT_TRUE(other.recorder->start().is_ok());
}

TEST(ActiveRecorderHandleTest, ARefusedSecondStartOfTheSameRecorderKeepsItsSession) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  FakeSession other(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());

  auto second = session.recorder->start();
  ASSERT_TRUE(second.is_err());
  EXPECT_EQ(second.unwrap_err(), "Recorder is already recording");

  EXPECT_TRUE(other.recorder->start().is_err());
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);
}

TEST(ActiveRecorderHandleTest, AFailedStartGivesTheSessionBack) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession failing(handle);
  FakeSession other(handle);
  failing.input->openError = "no microphone";

  ASSERT_TRUE(failing.recorder->start().is_err());

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_TRUE(other.recorder->start().is_ok());
}

TEST(ActiveRecorderHandleTest, ARecorderCanStartOnceThePreviousOneIsGone) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession successor(handle);
  {
    FakeSession gone(handle);
    ASSERT_TRUE(gone.recorder->start().is_ok());
  }

  ASSERT_TRUE(successor.recorder->start().is_ok());
}

TEST(ActiveRecorderHandleTest, RerouteReachesTheRecorderThatHoldsTheSession) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());

  EXPECT_TRUE(handle.rerouteInput().is_ok());

  EXPECT_EQ(session.input->openPurposes.size(), 2U);
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);
}

TEST(ActiveRecorderHandleTest, AFailedRerouteEndsTheSessionAndKeepsItsRecording) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  FakeSession session(handle);
  ASSERT_TRUE(session.recorder->start().is_ok());
  session.input->openError = "device gone";

  auto result = handle.rerouteInput();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_EQ(session.log.closedFiles.load(), 1);
  auto recording = handle.consumeLastRecordingResult();
  ASSERT_TRUE(recording.has_value());
  EXPECT_EQ(recording->paths.size(), 1U);
}

// Thread startup skew usually serializes a single two-thread run, so the race
// tests below repeat with a fresh handle/recorder and release both threads at
// once through an atomic start flag to actually hit concurrent interleavings.
constexpr int RACE_TEST_ITERATIONS = 200;

TEST(ActiveRecorderHandleTest, ConcurrentStopsCloseTheFileExactlyOnce) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    FakeSession session(handle);
    ASSERT_TRUE(session.recorder->start().is_ok());

    std::atomic<bool> startFlag{false};
    std::thread nativeStop([&] {
      while (!startFlag.load()) {}
      handle.stopAndReturnState();
    });
    std::thread recorderStop([&] {
      while (!startFlag.load()) {}
      session.recorder->stop();
    });
    startFlag.store(true);
    nativeStop.join();
    recorderStop.join();

    EXPECT_EQ(session.log.closedFiles.load(), 1) << "iteration " << iteration;
    EXPECT_EQ(handle.currentState(), RecorderState::Idle) << "iteration " << iteration;
    EXPECT_FALSE(handle.isRecordingOngoing()) << "iteration " << iteration;
    EXPECT_TRUE(handle.consumeLastRecordingResult().has_value()) << "iteration " << iteration;
  }
}

TEST(ActiveRecorderHandleTest, ConcurrentStartsAdmitOnlyOneRecorder) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    FakeSession first(handle, /*writesFile=*/false);
    FakeSession second(handle, /*writesFile=*/false);

    std::atomic<bool> startFlag{false};
    std::atomic<int> successes{0};
    std::thread startFirst([&] {
      while (!startFlag.load()) {}
      if (first.recorder->start().is_ok()) {
        successes.fetch_add(1);
      }
    });
    std::thread startSecond([&] {
      while (!startFlag.load()) {}
      if (second.recorder->start().is_ok()) {
        successes.fetch_add(1);
      }
    });
    startFlag.store(true);
    startFirst.join();
    startSecond.join();

    EXPECT_EQ(successes.load(), 1) << "iteration " << iteration;
  }
}

// NOLINTEND
