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
#include <vector>

using namespace audioapi;
using namespace audioapi::test_support;

// NOLINTBEGIN

namespace {

/// A recorder over a scripted input that reports to @p handle, writing its file through a fake
/// backend that lives as long as the recorder.
std::shared_ptr<AudioRecorder> makeRecorder(
    const std::shared_ptr<ActiveRecorderHandle> &handle,
    std::shared_ptr<FakeRecorderInput> input = std::make_shared<FakeRecorderInput>(),
    bool writesFile = true) {
  struct FileOutput {
    FakeEncoderLog log;
    PlatformFileBackend backend = makeFakeBackend(log);
  };
  auto fileOutput = std::make_shared<FileOutput>();
  auto recorder = std::shared_ptr<AudioRecorder>(
      new AudioRecorder(nullptr, std::move(input), handle, &fileOutput->backend),
      [fileOutput](AudioRecorder *recorder) { delete recorder; });
  if (writesFile) {
    EXPECT_TRUE(recorder->enableFileOutput(wavFileProperties()).is_ok());
  }
  return recorder;
}

} // namespace

TEST(ActiveRecorderHandleTest, EmptySlotReportsNoRecordingAndStopsNothing) {
  auto handle = std::make_shared<ActiveRecorderHandle>();

  EXPECT_EQ(handle->currentState(), RecorderState::Idle);
  EXPECT_FALSE(handle->isRecordingOngoing());
  EXPECT_TRUE(handle->stopAndReturnInfo().is_err());
  EXPECT_EQ(handle->stopAndReturnState(), RecorderState::Idle);
  EXPECT_FALSE(handle->consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, AStoppedRecorderIsNotOngoing) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto recorder = makeRecorder(handle);
  ASSERT_TRUE(recorder->start().is_ok());
  ASSERT_TRUE(handle->stopAndReturnInfo().is_ok());

  EXPECT_FALSE(handle->isRecordingOngoing());
  EXPECT_TRUE(handle->stopAndReturnInfo().is_err());
  EXPECT_EQ(handle->stopAndReturnState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, RecordingAndPausedCountAsOngoing) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto recorder = makeRecorder(handle);
  ASSERT_TRUE(recorder->start().is_ok());

  EXPECT_EQ(handle->currentState(), RecorderState::Recording);
  EXPECT_TRUE(handle->isRecordingOngoing());

  recorder->pause();
  EXPECT_EQ(handle->currentState(), RecorderState::Paused);
  EXPECT_TRUE(handle->isRecordingOngoing());
}

TEST(ActiveRecorderHandleTest, PauseAndResumeReachTheOccupant) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto recorder = makeRecorder(handle);

  EXPECT_EQ(handle->pause(), RecorderState::Idle);
  EXPECT_EQ(handle->resume(), RecorderState::Idle);

  ASSERT_TRUE(recorder->start().is_ok());
  EXPECT_EQ(handle->resume(), RecorderState::Recording);
  EXPECT_EQ(handle->pause(), RecorderState::Paused);
  EXPECT_TRUE(recorder->isPaused());

  EXPECT_EQ(handle->pause(), RecorderState::Paused);
  EXPECT_EQ(handle->resume(), RecorderState::Recording);
  EXPECT_TRUE(recorder->isRecording());
}

TEST(ActiveRecorderHandleTest, RefusedResumeKeepsSessionPausedAndReturnsTheReason) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto input = std::make_shared<FakeRecorderInput>();
  auto recorder = makeRecorder(handle, input);
  ASSERT_TRUE(recorder->start().is_ok());
  handle->pause();
  input->resumeError = "session activation refused";

  auto result = recorder->resume();

  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "session activation refused");
  EXPECT_TRUE(recorder->isPaused());
  EXPECT_EQ(handle->resume(), RecorderState::Paused);
}

TEST(ActiveRecorderHandleTest, StopKeepsTheRecordingForSingleConsumption) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto recorder = makeRecorder(handle);
  ASSERT_TRUE(recorder->start().is_ok());

  auto stopResult = handle->stopAndReturnInfo();
  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_FALSE(handle->isRecordingOngoing());

  auto result = handle->consumeLastRecordingResult();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->paths.size(), 1U);
  EXPECT_EQ(result->paths, stopResult.unwrap().paths);
  EXPECT_DOUBLE_EQ(result->duration, stopResult.unwrap().duration);

  EXPECT_FALSE(handle->consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, ARecorderStoppingItselfKeepsItsRecording) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto recorder = makeRecorder(handle);
  ASSERT_TRUE(recorder->start().is_ok());

  auto stopResult = recorder->stop();
  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_FALSE(handle->isRecordingOngoing());

  auto consumed = handle->consumeLastRecordingResult();
  ASSERT_TRUE(consumed.has_value());
  EXPECT_EQ(consumed->paths, stopResult.unwrap().paths);
}

TEST(ActiveRecorderHandleTest, StopWithoutFileOutputKeepsNothing) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto recorder = makeRecorder(handle, std::make_shared<FakeRecorderInput>(), /*writesFile=*/false);
  ASSERT_TRUE(recorder->start().is_ok());

  ASSERT_TRUE(handle->stopAndReturnInfo().is_ok());
  EXPECT_FALSE(handle->consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, ADestroyedRecorderFreesTheSlotAndKeepsItsRecording) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto successor = makeRecorder(handle);
  {
    auto destroyed = makeRecorder(handle);
    ASSERT_TRUE(destroyed->start().is_ok());
  }

  EXPECT_FALSE(handle->isRecordingOngoing());
  EXPECT_EQ(handle->pause(), RecorderState::Idle);
  EXPECT_EQ(handle->resume(), RecorderState::Idle);
  EXPECT_TRUE(handle->consumeLastRecordingResult().has_value());
  EXPECT_TRUE(successor->start().is_ok());
}

TEST(ActiveRecorderHandleTest, StoppingAnIdleRecorderLeavesTheOccupantAlone) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto currentInput = std::make_shared<FakeRecorderInput>();
  auto current = makeRecorder(handle, currentInput);
  auto other = makeRecorder(handle);
  ASSERT_TRUE(current->start().is_ok());

  auto result = other->stop();
  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "Recorder is not in recording state.");
  EXPECT_EQ(handle->currentState(), RecorderState::Recording);
  EXPECT_EQ(currentInput->closeCount, 0);
}

TEST(ActiveRecorderHandleTest, AnotherRecorderCannotStartWhileASessionIsActive) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto current = makeRecorder(handle);
  auto otherInput = std::make_shared<FakeRecorderInput>();
  auto other = makeRecorder(handle, otherInput);
  ASSERT_TRUE(current->start().is_ok());

  auto otherResult = other->start();
  ASSERT_TRUE(otherResult.is_err());
  EXPECT_EQ(otherResult.unwrap_err(), "Another recording is already in progress");
  EXPECT_TRUE(otherInput->openPurposes.empty());
  EXPECT_EQ(handle->currentState(), RecorderState::Recording);

  current->pause();
  EXPECT_TRUE(other->start().is_err());
  EXPECT_EQ(handle->currentState(), RecorderState::Paused);

  ASSERT_TRUE(current->stop().is_ok());
  ASSERT_TRUE(other->start().is_ok());
  EXPECT_EQ(handle->currentState(), RecorderState::Recording);
}

TEST(ActiveRecorderHandleTest, StartingARecordingRecorderAgainIsRefused) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto recorder = makeRecorder(handle);
  ASSERT_TRUE(recorder->start().is_ok());

  auto second = recorder->start();
  ASSERT_TRUE(second.is_err());
  EXPECT_EQ(second.unwrap_err(), "Recorder is already recording");
  EXPECT_EQ(handle->currentState(), RecorderState::Recording);
}

// A native stop (a notification action, a session deactivation) must find a start still waiting
// for its route, and that start must not record once the route arrives.
TEST(ActiveRecorderHandleTest, ANativeStopWhileAStartWaitsForItsRouteCancelsTheStart) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto input = std::make_shared<FakeRecorderInput>();
  auto recorder = makeRecorder(handle, input);
  input->duringAcquireRoute = [&handle] {
    EXPECT_EQ(handle->currentState(), RecorderState::Recording);
    EXPECT_EQ(handle->stopAndReturnState(), RecorderState::Idle);
  };

  EXPECT_TRUE(recorder->start().is_err());

  EXPECT_TRUE(recorder->isIdle());
  EXPECT_TRUE(input->openPurposes.empty());
  EXPECT_EQ(input->routesReleased.load(), input->routesAcquired.load());
  EXPECT_FALSE(handle->isRecordingOngoing());
  EXPECT_FALSE(handle->consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, AStartThatCannotOpenTheInputFreesTheSlot) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto input = std::make_shared<FakeRecorderInput>();
  auto failing = makeRecorder(handle, input);
  input->openError = "device busy";

  EXPECT_TRUE(failing->start().is_err());

  EXPECT_FALSE(handle->isRecordingOngoing());
  EXPECT_TRUE(makeRecorder(handle)->start().is_ok());
}

TEST(ActiveRecorderHandleTest, AStartThatFailsAfterOpeningTheInputFreesTheSlot) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto input = std::make_shared<FakeRecorderInput>();
  auto failing = makeRecorder(handle, input);
  input->startError = "engine refused";

  EXPECT_TRUE(failing->start().is_err());

  EXPECT_FALSE(handle->isRecordingOngoing());
  EXPECT_FALSE(handle->consumeLastRecordingResult().has_value());
  EXPECT_TRUE(makeRecorder(handle)->start().is_ok());
}

TEST(ActiveRecorderHandleTest, ALateRecordingOfAnOlderSessionDoesNotReplaceANewerOne) {
  auto handle = std::make_shared<ActiveRecorderHandle>();
  auto older = makeRecorder(handle);
  auto newer = makeRecorder(handle);
  auto olderSession = handle->claim(older);
  ASSERT_TRUE(olderSession.is_ok());
  handle->release(older);
  auto newerSession = handle->claim(newer);
  ASSERT_TRUE(newerSession.is_ok());
  handle->release(newer);

  // The newer session's file closes first, as when the older one finalizes a long recording.
  handle->keepRecording(newerSession.unwrap(), FileInfo{.paths = {"file:///newer.wav"}});
  handle->keepRecording(olderSession.unwrap(), FileInfo{.paths = {"file:///older.wav"}});

  auto kept = handle->consumeLastRecordingResult();
  ASSERT_TRUE(kept.has_value());
  EXPECT_EQ(kept->paths, (std::vector<std::string>{"file:///newer.wav"}));

  handle->keepRecording(olderSession.unwrap(), FileInfo{.paths = {"file:///older.wav"}});
  EXPECT_FALSE(handle->consumeLastRecordingResult().has_value());
}

// Thread startup skew usually serializes a single two-thread run, so the race
// tests below repeat with a fresh handle/recorder and release both threads at
// once through an atomic start flag to actually hit concurrent interleavings.
constexpr int RACE_TEST_ITERATIONS = 200;

TEST(ActiveRecorderHandleTest, ConcurrentNativeAndRecorderStopsCloseTheFileExactlyOnce) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handle = std::make_shared<ActiveRecorderHandle>();
    auto input = std::make_shared<FakeRecorderInput>();
    auto recorder = makeRecorder(handle, input);
    ASSERT_TRUE(recorder->start().is_ok());

    std::atomic<bool> startFlag{false};
    std::thread nativeStop([&] {
      while (!startFlag.load()) {}
      handle->stopAndReturnState();
    });
    std::thread hostObjectStop([&] {
      while (!startFlag.load()) {}
      recorder->stop();
    });
    startFlag.store(true);
    nativeStop.join();
    hostObjectStop.join();

    EXPECT_EQ(input->closeCount, 1) << "iteration " << iteration;
    EXPECT_EQ(handle->currentState(), RecorderState::Idle) << "iteration " << iteration;
  }
}

TEST(ActiveRecorderHandleTest, ConcurrentStartsAdmitOnlyOneRecorder) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handle = std::make_shared<ActiveRecorderHandle>();
    auto first = makeRecorder(handle);
    auto second = makeRecorder(handle);

    std::atomic<bool> startFlag{false};
    std::atomic<int> successes{0};
    std::thread startFirst([&] {
      while (!startFlag.load()) {}
      if (first->start().is_ok()) {
        successes.fetch_add(1);
      }
    });
    std::thread startSecond([&] {
      while (!startFlag.load()) {}
      if (second->start().is_ok()) {
        successes.fetch_add(1);
      }
    });
    startFlag.store(true);
    startFirst.join();
    startSecond.join();

    EXPECT_EQ(successes.load(), 1) << "iteration " << iteration;
  }
}

// The occupant's owner drops it while another recorder claims: the claim must not end up as its
// last owner, whose destructor releases the slot, under the handle's mutex.
TEST(ActiveRecorderHandleTest, AClaimRacingTheOccupantsDestructionDoesNotDeadlock) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handle = std::make_shared<ActiveRecorderHandle>();
    auto occupant = makeRecorder(handle);
    auto claimant = makeRecorder(handle);
    ASSERT_TRUE(occupant->start().is_ok());

    std::atomic<bool> startFlag{false};
    std::thread destroy([&] {
      while (!startFlag.load()) {}
      occupant.reset();
    });
    std::thread claim([&] {
      while (!startFlag.load()) {}
      claimant->start();
    });
    startFlag.store(true);
    destroy.join();
    claim.join();

    EXPECT_EQ(handle->currentState(), claimant->getState()) << "iteration " << iteration;
  }
}

// NOLINTEND
