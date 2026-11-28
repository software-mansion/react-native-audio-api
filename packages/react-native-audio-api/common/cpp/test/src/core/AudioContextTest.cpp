#include <audioapi/core/AudioContext.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>
#include <mutex>
#include <new>
#include <optional>

using namespace audioapi;

// NOLINTBEGIN

namespace {

class FakeAudioPlayer : public AudioPlayer {
 public:
  using AudioPlayer::AudioPlayer;

  bool start() override {
    isRunning_->store(true);
    return true;
  }
  void stop() override {
    isRunning_->store(false);
    ++stopCount;
  }
  bool resume() override {
    return start();
  }
  void suspend() override {
    stop();
  }
  void cleanup() override {}
  [[nodiscard]] double getBaseLatency() const override {
    return 0.0;
  }
  [[nodiscard]] double getOutputLatency() const override {
    return 0.0;
  }
  [[nodiscard]] bool isRunning() const override {
    return isRunning_->load();
  }
  std::optional<OutputPresentation> getOutputPresentation() override {
    return std::nullopt;
  }

  [[nodiscard]] std::shared_ptr<AudioContext> boundContext() const {
    return context_.lock();
  }

  int stopCount = 0;
};

bool failPlayerCreation = false;
std::shared_ptr<FakeAudioPlayer> lastPlayer;

/// Exposes the driver mutex so a test can start the context the way its lifecycle lane does.
class TestAudioContext : public AudioContext {
 public:
  TestAudioContext()
      : AudioContext(
            44100,
            AndroidOutputProfile::Media,
            std::make_shared<MockAudioEventHandlerRegistry>(),
            AudioContextLatencyHint::INTERACTIVE) {}

  bool startDriver() {
    std::scoped_lock lock(driverMutex_);
    return start();
  }
};

} // namespace

std::shared_ptr<AudioPlayer> audioapi::createPlatformAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
    float sampleRate,
    std::atomic<uint32_t> &currentRenders,
    std::mutex *driverMutex,
    AudioContextLatencyHint latencyHint,
    AndroidOutputProfile) {
  if (failPlayerCreation) {
    throw std::bad_alloc();
  }
  lastPlayer = std::make_shared<FakeAudioPlayer>(
      renderAudio, sampleRate, currentRenders, driverMutex, latencyHint);
  return lastPlayer;
}

class AudioContextTest : public ::testing::Test {
 protected:
  void TearDown() override {
    failPlayerCreation = false;
    lastPlayer.reset();
  }
};

// RNAA-608: a failed player creation must throw from construction, not crash in the destructor of
// a context that never got a player.
TEST_F(AudioContextTest, PlayerCreationFailureThrowsFromConstruction) {
  failPlayerCreation = true;
  EXPECT_THROW(std::make_shared<TestAudioContext>(), std::bad_alloc);
}

TEST_F(AudioContextTest, DestroyingAnUnclosedContextStopsThePlayer) {
  std::make_shared<TestAudioContext>().reset();
  EXPECT_EQ(lastPlayer->stopCount, 1);
}

TEST_F(AudioContextTest, StartingBindsThePlayerToTheContext) {
  auto context = std::make_shared<TestAudioContext>();
  EXPECT_EQ(lastPlayer->boundContext(), nullptr);

  EXPECT_TRUE(context->startDriver());

  EXPECT_EQ(lastPlayer->boundContext(), context);
}

// NOLINTEND
