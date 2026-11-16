#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <gtest/gtest.h>
#include <test/src/core/inputs/FakeRecorderInput.h>
#include <test/src/utils/FakeFileBackend.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <utility>

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

/// A recorder over a scripted input, writing its file through a fake backend that lives as long
/// as the recorder.
std::shared_ptr<AudioRecorder> makeRecorder(
    std::shared_ptr<FakeRecorderInput> input = std::make_shared<FakeRecorderInput>(),
    bool writesFile = true) {
  struct FileOutput {
    FakeEncoderLog log;
    PlatformFileBackend backend = makeFakeBackend(log);
  };
  auto fileOutput = std::make_shared<FileOutput>();
  auto recorder = std::shared_ptr<AudioRecorder>(
      new AudioRecorder(nullptr, std::move(input), &fileOutput->backend),
      [fileOutput](AudioRecorder *recorder) { delete recorder; });
  if (writesFile) {
    EXPECT_TRUE(recorder->enableFileOutput(wavFileProperties()).is_ok());
  }
  return recorder;
}

} // namespace

TEST(ActiveRecorderHandleTest, EmptySlotReportsNoRecordingAndStopsNothing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;

  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_TRUE(handle.stopAndReturnInfo().is_err());
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, IdleRecorderIsNotOngoing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = makeRecorder();
  ASSERT_TRUE(handle.tryStart(recorder).is_ok());
  ASSERT_TRUE(handle.stopAndReturnInfo().is_ok());

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_TRUE(handle.stopAndReturnInfo().is_err());
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, RecordingAndPausedCountAsOngoing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = makeRecorder();
  ASSERT_TRUE(handle.tryStart(recorder).is_ok());

  EXPECT_EQ(handle.currentState(), RecorderState::Recording);
  EXPECT_TRUE(handle.isRecordingOngoing());

  recorder->pause();
  EXPECT_EQ(handle.currentState(), RecorderState::Paused);
  EXPECT_TRUE(handle.isRecordingOngoing());
}

TEST(ActiveRecorderHandleTest, PauseAndResumeActOnlyInMatchingStates) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = makeRecorder();

  EXPECT_EQ(handle.pause(), RecorderState::Idle);
  EXPECT_EQ(handle.resume(), RecorderState::Idle);

  ASSERT_TRUE(handle.tryStart(recorder).is_ok());
  EXPECT_EQ(handle.resume(), RecorderState::Recording);
  EXPECT_EQ(handle.pause(), RecorderState::Paused);
  EXPECT_TRUE(recorder->isPaused());

  EXPECT_EQ(handle.pause(), RecorderState::Paused);
  EXPECT_EQ(handle.resume(), RecorderState::Recording);
  EXPECT_TRUE(recorder->isRecording());
}

TEST(ActiveRecorderHandleTest, PauseAndResumeWithExpectedIgnoreForeignRecorder) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto current = makeRecorder();
  auto other = makeRecorder();
  ASSERT_TRUE(handle.tryStart(current).is_ok());

  EXPECT_EQ(handle.pause(other), RecorderState::Idle);
  EXPECT_TRUE(current->isRecording());

  EXPECT_EQ(handle.pause(current), RecorderState::Paused);
  EXPECT_TRUE(current->isPaused());

  auto foreignResume = handle.resume(other);
  ASSERT_TRUE(foreignResume.is_err());
  EXPECT_EQ(foreignResume.unwrap_err(), "Recorder is not in recording state.");
  EXPECT_TRUE(current->isPaused());

  EXPECT_TRUE(handle.resume(current).is_ok());
  EXPECT_TRUE(current->isRecording());
}

TEST(ActiveRecorderHandleTest, RefusedResumeKeepsSessionPausedAndReturnsTheReason) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto input = std::make_shared<FakeRecorderInput>();
  auto recorder = makeRecorder(input);
  ASSERT_TRUE(handle.tryStart(recorder).is_ok());
  handle.pause();
  input->resumeError = "session activation refused";

  auto result = handle.resume(recorder);

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "session activation refused");
  EXPECT_TRUE(recorder->isPaused());
  EXPECT_EQ(handle.resume(), RecorderState::Paused);
}

TEST(ActiveRecorderHandleTest, StopStashesResultForSingleConsumption) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = makeRecorder();
  ASSERT_TRUE(handle.tryStart(recorder).is_ok());

  auto stopResult = handle.stopAndReturnInfo();
  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_FALSE(handle.isRecordingOngoing());

  auto result = handle.consumeLastRecordingResult();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->paths.size(), 1U);
  EXPECT_EQ(result->paths, stopResult.unwrap().paths);
  EXPECT_DOUBLE_EQ(result->duration, stopResult.unwrap().duration);

  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, StopLeavesResultForConsumeLastRecordingResult) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = makeRecorder();
  ASSERT_TRUE(handle.tryStart(recorder).is_ok());

  auto stopResult = handle.stopAndReturnInfo();
  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_FALSE(stopResult.unwrap().paths.empty());
  EXPECT_FALSE(handle.isRecordingOngoing());

  auto consumed = handle.consumeLastRecordingResult();
  ASSERT_TRUE(consumed.has_value());
  EXPECT_EQ(consumed->paths, stopResult.unwrap().paths);
}

TEST(ActiveRecorderHandleTest, StopWithoutFileOutputStashesNothing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = makeRecorder(std::make_shared<FakeRecorderInput>(), /*writesFile=*/false);
  ASSERT_TRUE(handle.tryStart(recorder).is_ok());

  ASSERT_TRUE(handle.stopAndReturnInfo().is_ok());
  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, ExpiredRecorderReportsNoRecording) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  {
    auto recorder = makeRecorder();
    ASSERT_TRUE(handle.tryStart(recorder).is_ok());
  }

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_EQ(handle.pause(), RecorderState::Idle);
  EXPECT_EQ(handle.resume(), RecorderState::Idle);
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, StopAndReturnInfoWithExpectedIgnoresForeignRecorder) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto currentInput = std::make_shared<FakeRecorderInput>();
  auto current = makeRecorder(currentInput);
  auto other = makeRecorder();
  ASSERT_TRUE(handle.tryStart(current).is_ok());

  auto result = handle.stopAndReturnInfo(other);
  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "Recorder is not in recording state.");
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);
  EXPECT_EQ(currentInput->closeCount, 0);

  ASSERT_TRUE(handle.stopAndReturnInfo(current).is_ok());
  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
  EXPECT_EQ(currentInput->closeCount, 1);
}

TEST(ActiveRecorderHandleTest, TryStartFailsWhenAnotherSessionIsInProgress) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto current = makeRecorder();
  auto other = makeRecorder();
  ASSERT_TRUE(handle.tryStart(current).is_ok());

  auto otherResult = handle.tryStart(other);
  ASSERT_TRUE(otherResult.is_err());
  EXPECT_EQ(otherResult.unwrap_err(), "Another recording is already in progress");
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);

  current->pause();
  auto pausedOtherResult = handle.tryStart(other);
  ASSERT_TRUE(pausedOtherResult.is_err());
  EXPECT_EQ(handle.currentState(), RecorderState::Paused);

  ASSERT_TRUE(handle.stopAndReturnInfo(current).is_ok());
  ASSERT_TRUE(handle.tryStart(other).is_ok());
}

TEST(ActiveRecorderHandleTest, TryStartOnSameRecorderReachesStart) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = makeRecorder();
  ASSERT_TRUE(handle.tryStart(recorder).is_ok());

  auto second = handle.tryStart(recorder);
  ASSERT_TRUE(second.is_err());
  EXPECT_EQ(second.unwrap_err(), "Recorder is already recording");
}

TEST(ActiveRecorderHandleTest, TryStartSucceedsWhenPreviousRecorderExpired) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto successor = makeRecorder();
  {
    auto expired = makeRecorder();
    ASSERT_TRUE(handle.tryStart(expired).is_ok());
  }

  ASSERT_TRUE(handle.tryStart(successor).is_ok());
}

// Thread startup skew usually serializes a single two-thread run, so the race
// tests below repeat with a fresh handle/recorder and release both threads at
// once through an atomic start flag to actually hit concurrent interleavings.
constexpr int RACE_TEST_ITERATIONS = 200;

TEST(ActiveRecorderHandleTest, ConcurrentStopsCloseTheFileExactlyOnce) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    auto input = std::make_shared<FakeRecorderInput>();
    auto recorder = makeRecorder(input);
    ASSERT_TRUE(handle.tryStart(recorder).is_ok());

    std::atomic<bool> startFlag{false};
    std::thread nativeStop([&] {
      while (!startFlag.load()) {}
      handle.stopAndReturnState();
    });
    std::thread jsStop([&] {
      while (!startFlag.load()) {}
      handle.stopAndReturnInfo();
    });
    startFlag.store(true);
    nativeStop.join();
    jsStop.join();

    EXPECT_EQ(input->closeCount, 1) << "iteration " << iteration;
  }
}

TEST(ActiveRecorderHandleTest, ConcurrentTargetedAndUntargetedStopsCloseTheFileOnce) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    auto input = std::make_shared<FakeRecorderInput>();
    auto recorder = makeRecorder(input);
    ASSERT_TRUE(handle.tryStart(recorder).is_ok());

    std::atomic<bool> startFlag{false};
    std::thread hostObjectStop([&] {
      while (!startFlag.load()) {}
      handle.stopAndReturnInfo(recorder);
    });
    std::thread nativeStop([&] {
      while (!startFlag.load()) {}
      handle.stopAndReturnState();
    });
    startFlag.store(true);
    hostObjectStop.join();
    nativeStop.join();

    EXPECT_EQ(input->closeCount, 1) << "iteration " << iteration;
    EXPECT_EQ(handle.currentState(), RecorderState::Idle) << "iteration " << iteration;
  }
}

TEST(ActiveRecorderHandleTest, ConcurrentTryStartAdmitsOnlyOneRecorder) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    auto first = makeRecorder();
    auto second = makeRecorder();

    std::atomic<bool> startFlag{false};
    std::atomic<int> successes{0};
    std::thread tryFirst([&] {
      while (!startFlag.load()) {}
      if (handle.tryStart(first).is_ok()) {
        successes.fetch_add(1);
      }
    });
    std::thread trySecond([&] {
      while (!startFlag.load()) {}
      if (handle.tryStart(second).is_ok()) {
        successes.fetch_add(1);
      }
    });
    startFlag.store(true);
    tryFirst.join();
    trySecond.join();

    EXPECT_EQ(successes.load(), 1) << "iteration " << iteration;
  }
}

// NOLINTEND
