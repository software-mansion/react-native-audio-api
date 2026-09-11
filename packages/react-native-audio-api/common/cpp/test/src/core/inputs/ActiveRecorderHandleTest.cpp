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
  EXPECT_EQ(handle.stopActiveRecording(), RecorderState::Idle);
  EXPECT_FALSE(handle.takeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, IdleRecorderIsNotOngoing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  handle.setRecorder(recorder);

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_EQ(handle.stopActiveRecording(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, RecordingAndPausedCountAsOngoing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  handle.setRecorder(recorder);

  recorder->start("");
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
  handle.setRecorder(recorder);

  EXPECT_EQ(handle.pauseActiveRecording(), RecorderState::Idle);
  EXPECT_EQ(handle.resumeActiveRecording(), RecorderState::Idle);

  recorder->start("");
  EXPECT_EQ(handle.resumeActiveRecording(), RecorderState::Recording);
  EXPECT_EQ(handle.pauseActiveRecording(), RecorderState::Paused);
  EXPECT_TRUE(recorder->isPaused());

  EXPECT_EQ(handle.pauseActiveRecording(), RecorderState::Paused);
  EXPECT_EQ(handle.resumeActiveRecording(), RecorderState::Recording);
  EXPECT_TRUE(recorder->isRecording());
}

TEST(ActiveRecorderHandleTest, StopStashesResultForSingleConsumption) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  handle.setRecorder(recorder);
  recorder->start("");

  EXPECT_EQ(handle.stopActiveRecording(), RecorderState::Idle);
  EXPECT_FALSE(handle.isRecordingOngoing());

  auto result = handle.takeLastRecordingResult();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result->paths, recorder->stopPaths);
  EXPECT_DOUBLE_EQ(result->size, 1.5);
  EXPECT_DOUBLE_EQ(result->duration, 10.0);

  EXPECT_FALSE(handle.takeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, StopWithoutFileOutputStashesNothing) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto recorder = std::make_shared<FakeAudioRecorder>();
  recorder->stopPaths.clear();
  handle.setRecorder(recorder);
  recorder->start("");

  EXPECT_EQ(handle.stopActiveRecording(), RecorderState::Idle);
  EXPECT_FALSE(handle.takeLastRecordingResult().has_value());
}

TEST(ActiveRecorderHandleTest, ExpiredRecorderReportsNoRecording) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  {
    auto recorder = std::make_shared<FakeAudioRecorder>();
    handle.setRecorder(recorder);
    recorder->start("");
  }

  EXPECT_FALSE(handle.isRecordingOngoing());
  EXPECT_EQ(handle.pauseActiveRecording(), RecorderState::Idle);
  EXPECT_EQ(handle.resumeActiveRecording(), RecorderState::Idle);
  EXPECT_EQ(handle.stopActiveRecording(), RecorderState::Idle);
}

TEST(ActiveRecorderHandleTest, ClearRecorderIgnoresForeignPointer) {
  auto handleOwner = ActiveRecorderHandleTestPeer::createHandle();
  ActiveRecorderHandle &handle = *handleOwner;
  auto current = std::make_shared<FakeAudioRecorder>();
  auto other = std::make_shared<FakeAudioRecorder>();
  handle.setRecorder(current);
  current->start("");

  // The host object of a replaced recorder is collected long after its successor
  // registered; its late destructor must leave the live recorder in the slot.
  handle.clearRecorder(other.get());
  EXPECT_EQ(handle.currentState(), RecorderState::Recording);

  handle.clearRecorder(current.get());
  EXPECT_EQ(handle.currentState(), RecorderState::Idle);
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
    handle.setRecorder(recorder);
    recorder->start("");

    std::atomic<bool> startFlag{false};
    std::thread nativeStop([&] {
      while (!startFlag.load()) {}
      handle.stopActiveRecording();
    });
    std::thread jsStop([&] {
      while (!startFlag.load()) {}
      recorder->stop();
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
    handle.setRecorder(recorder);
    recorder->start("");

    std::atomic<bool> startFlag{false};
    std::thread hostObjectClear([&] {
      while (!startFlag.load()) {}
      handle.clearRecorder(recorder.get());
    });
    std::thread nativeStop([&] {
      while (!startFlag.load()) {}
      handle.stopActiveRecording();
    });
    startFlag.store(true);
    hostObjectClear.join();
    nativeStop.join();

    EXPECT_LE(recorder->stopCount, 1) << "iteration " << iteration;
  }
}

// NOLINTEND
