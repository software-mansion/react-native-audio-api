#include <audioapi/core/AudioContext.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

// Tests never run a platform driver; `AudioContext::initialize()` is the only caller.
std::shared_ptr<AudioPlayer> audioapi::createPlatformAudioPlayer(
    const std::function<void(DSPAudioBuffer *, int)> &,
    float,
    int,
    std::atomic<uint32_t> &,
    std::weak_ptr<AudioContext>,
    std::mutex *,
    AudioContextLatencyHint,
    AndroidOutputProfile) {
  return nullptr;
}

// RNAA-608: host-object construction can throw between creating the context and `initialize()`;
// the context is then destroyed without a player.
TEST(AudioContextTest, DestroyedBeforeInitializeDoesNotTouchPlayer) {
  auto context = std::make_shared<AudioContext>(
      44100,
      AndroidOutputProfile::Media,
      std::make_shared<MockAudioEventHandlerRegistry>(),
      AudioContextLatencyHint::INTERACTIVE);

  EXPECT_EQ(context->getState(), ContextState::SUSPENDED);
  context.reset();
}

// NOLINTEND
