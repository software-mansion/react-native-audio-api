#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <gtest/gtest.h>
#include "MockAudioEventHandlerRegistry.h"

class OscillatorTest : public ::testing::Test {
 protected:
  std::shared_ptr<audioapi::IAudioEventHandlerRegistry> eventRegistry;
  std::unique_ptr<audioapi::OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_unique<audioapi::OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
  }
};

TEST_F(OscillatorTest, OscillatorCanBeCreated) {
  auto osc = context->createOscillator();
  ASSERT_NE(osc, nullptr);
}
