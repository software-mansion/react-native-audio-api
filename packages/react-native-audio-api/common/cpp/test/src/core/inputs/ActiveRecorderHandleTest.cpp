#include <audioapi/core/inputs/ActiveRecorderHandle.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <gtest/gtest.h>
#include <atomic>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <vector>

using namespace audioapi;

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

class FakeAudioRecorder : public AudioRecorder {
 public:
  FakeAudioRecorder() : AudioRecorder(nullptr) {}

  std::vector<std::string> stopPaths{"file:///tmp/recording.m4a"};
  std::atomic<int> stopCount{0};

  Result<NoneType, std::string> start(const std::string &) override {
    if (state_ != RecorderState::Idle) {
      return Err(std::string("Recorder is already recording"));
    }
    state_ = RecorderState::Recording;
    return Ok(None);
  }

  // Mirrors AndroidAudioRecorder::stop(): under its locks exactly one caller
  // transitions out of a non-idle state and closes the file; the loser errs.
  Result<std::tuple<std::vector<std::string>, double, double>, std::string> stop() override {
    if (state_.exchange(RecorderState::Idle) == RecorderState::Idle) {
      return Err(std::string("Recorder is not in recording state."));
    }
    stopCount += 1;
    return Ok(std::make_tuple(stopPaths, 1.5, 10.0));
  }

  Result<NoneType, std::string> enableFileOutput(std::shared_ptr<AudioFileProperties>) override {
    return Ok(None);
  }
  void disableFileOutput() override {}

  void pause() override {
    state_ = RecorderState::Paused;
  }
  void resume() override {
    state_ = RecorderState::Recording;
  }

  void connect(const std::shared_ptr<utils::graph::NodeHandle> &) override {}
  void disconnect() override {}

  Result<NoneType, std::string> setOnAudioReadyCallback(float, size_t, int, uint64_t) override {
    return Ok(None);
  }
  void clearOnAudioReadyCallback() override {}

  bool isRecording() const override {
    return state_ == RecorderState::Recording;
  }
  bool isPaused() const override {
    return state_ == RecorderState::Paused;
  }
  bool isIdle() const override {
    return state_ == RecorderState::Idle;
  }

  [[nodiscard]] double getInputLatency() const override {
    return 0.0;
  }
};

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
  auto recorder = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());
  ASSERT_TRUE(handle.stopAndReturnInfo().is_ok());

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_TRUE(handle.stopAndReturnInfo().is_err());
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, RecordingAndPausedCountAsOngoing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());

  EXPECT_EQ(handle.currentState(), RecorderState::Recording);
  EXPECT_TRUE(handle.isRecordingOngoing());

  recorder->pause();
  EXPECT_EQ(handle.currentState(), RecorderState::Paused);
  EXPECT_TRUE(handle.isRecordingOngoing());
}

TEST(ActiveRecorderHandleTest, PauseAndResumeActOnlyInMatchingStates) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();

  EXPECT_EQ(handle.pause(), RecorderState::Idle);
  EXPECT_EQ(handle.resume(), RecorderState::Idle);

  ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());
  EXPECT_EQ(handle.resume(), RecorderState::Recording);
  EXPECT_EQ(handle.pause(), RecorderState::Paused);
  EXPECT_TRUE(recorder->isPaused());

  EXPECT_EQ(handle.pause(), RecorderState::Paused);
  EXPECT_EQ(handle.resume(), RecorderState::Recording);
  EXPECT_TRUE(recorder->isRecording());
}

TEST(ActiveRecorderHandleTest, StopStashesResultForSingleConsumption) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());

  ASSERT_TRUE(handle.stopAndReturnInfo().is_ok());
  EXPECT_FALSE(handle.isRecordingOngoing());

  auto result = handle.consumeLastRecordingResult();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->paths, recorder->stopPaths);
  EXPECT_DOUBLE_EQ(result->size, 1.5);
  EXPECT_DOUBLE_EQ(result->duration, 10.0);

  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, StopLeavesResultForConsumeLastRecordingResult) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());

  auto stopResult = handle.stopAndReturnInfo();
  ASSERT_TRUE(stopResult.is_ok());
  EXPECT_EQ(stopResult.unwrap().paths, recorder->stopPaths);
  EXPECT_FALSE(handle.isRecordingOngoing());

  auto consumed = handle.consumeLastRecordingResult();
  ASSERT_TRUE(consumed.has_value());
  EXPECT_EQ(consumed->paths, recorder->stopPaths);
}

TEST(ActiveRecorderHandleTest, StopWithoutFileOutputStashesNothing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  recorder->stopPaths.clear();
  ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());

  ASSERT_TRUE(handle.stopAndReturnInfo().is_ok());
  EXPECT_FALSE(handle.consumeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, ExpiredRecorderReportsNoRecording) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  {
    auto recorder = std::make_shared<FakeAudioRecorder>();
    ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());
  }

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_EQ(handle.pause(), RecorderState::Idle);
  EXPECT_EQ(handle.resume(), RecorderState::Idle);
  EXPECT_EQ(handle.stopAndReturnState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, ClearRecorderIgnoresForeignPointer) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto current = std::make_shared<FakeAudioRecorder>();
  auto other = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(current, "").is_ok());

  // The host object of a replaced recorder is collected long after its successor
  // registered; its late destructor must leave the live recorder in the slot.
  handle.clearRecorder(other);
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);

  handle.clearRecorder(current);
  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, ClearRecorderWithoutExpectedClearsAnyone) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto current = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(current, "").is_ok());

  handle.clearRecorder();
  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
  EXPECT_TRUE(current->isRecording());
}

TEST(ActiveRecorderHandleTest, StopAndReturnInfoWithExpectedIgnoresForeignRecorder) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto current = std::make_shared<FakeAudioRecorder>();
  auto other = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(current, "").is_ok());

  auto result = handle.stopAndReturnInfo(other);
  ASSERT_TRUE(result.is_err());
  EXPECT_EQ(result.unwrap_err(), "Recorder is not in recording state.");
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);
  EXPECT_EQ(current->stopCount, 0);

  ASSERT_TRUE(handle.stopAndReturnInfo(current).is_ok());
  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
  EXPECT_EQ(current->stopCount, 1);
}

TEST(ActiveRecorderHandleTest, TryStartFailsWhenAnotherSessionIsInProgress) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto current = std::make_shared<FakeAudioRecorder>();
  auto other = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(current, "").is_ok());

  auto otherResult = handle.tryStart(other, "");
  ASSERT_TRUE(otherResult.is_err());
  EXPECT_EQ(otherResult.unwrap_err(), "Another recording is already in progress");
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);

  current->pause();
  auto pausedOtherResult = handle.tryStart(other, "");
  ASSERT_TRUE(pausedOtherResult.is_err());
  EXPECT_EQ(handle.currentState(), RecorderState::Paused);

  handle.clearRecorder(current);
  ASSERT_TRUE(handle.tryStart(other, "").is_ok());
}

TEST(ActiveRecorderHandleTest, TryStartOnSameRecorderReachesStart) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());

  auto second = handle.tryStart(recorder, "");
  ASSERT_TRUE(second.is_err());
  EXPECT_EQ(second.unwrap_err(), "Recorder is already recording");
}

TEST(ActiveRecorderHandleTest, TryStartSucceedsWhenPreviousRecorderExpired) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto successor = std::make_shared<FakeAudioRecorder>();
  {
    auto expired = std::make_shared<FakeAudioRecorder>();
    ASSERT_TRUE(handle.tryStart(expired, "").is_ok());
  }

  ASSERT_TRUE(handle.tryStart(successor, "").is_ok());
}

// Thread startup skew usually serializes a single two-thread run, so the race
// tests below repeat with a fresh handle/recorder and release both threads at
// once through an atomic start flag to actually hit concurrent interleavings.
constexpr int RACE_TEST_ITERATIONS = 200;

TEST(ActiveRecorderHandleTest, ConcurrentStopsCloseTheFileExactlyOnce) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    auto recorder = std::make_shared<FakeAudioRecorder>();
    ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());

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

    EXPECT_EQ(recorder->stopCount, 1) << "iteration " << iteration;
  }
}

TEST(ActiveRecorderHandleTest, ConcurrentClearAndStopNeverCloseTheFileTwice) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    auto recorder = std::make_shared<FakeAudioRecorder>();
    ASSERT_TRUE(handle.tryStart(recorder, "").is_ok());

    std::atomic<bool> startFlag{false};
    std::thread hostObjectClear([&] {
      while (!startFlag.load()) {}
      handle.clearRecorder(recorder);
    });
    std::thread nativeStop([&] {
      while (!startFlag.load()) {}
      handle.stopAndReturnState();
    });
    startFlag.store(true);
    hostObjectClear.join();
    nativeStop.join();

    EXPECT_LE(recorder->stopCount, 1) << "iteration " << iteration;
  }
}

TEST(ActiveRecorderHandleTest, ConcurrentTryStartAdmitsOnlyOneRecorder) {
  for (int iteration = 0; iteration < RACE_TEST_ITERATIONS; ++iteration) {
    auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
    ActiveRecorderHandle &handle = *handleOwner;
    auto first = std::make_shared<FakeAudioRecorder>();
    auto second = std::make_shared<FakeAudioRecorder>();

    std::atomic<bool> startFlag{false};
    std::atomic<int> successes{0};
    std::thread tryFirst([&] {
      while (!startFlag.load()) {}
      if (handle.tryStart(first, "").is_ok()) {
        successes.fetch_add(1);
      }
    });
    std::thread trySecond([&] {
      while (!startFlag.load()) {}
      if (handle.tryStart(second, "").is_ok()) {
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
