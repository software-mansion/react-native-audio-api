#pragma once

#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <gtest/gtest.h>
#include <test/MockAudioEventHandlerRegistry.h>
#include <audioapi/core/effects/BiquadFilterNode.h>
#include <memory>

namespace audioapi {
class BiquadFilterTest : public ::testing::Test {
 protected:
  std::shared_ptr<IAudioEventHandlerRegistry> eventRegistry;
  std::unique_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_unique<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
  }
};
} // namespace audioapi
