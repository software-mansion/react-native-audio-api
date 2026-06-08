#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/PeriodicWave.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/types/NodeOptions.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <cmath>
#include <memory>

using namespace audioapi;

// NOLINTBEGIN

class OscillatorTest : public ::testing::Test {
 protected:
  std::shared_ptr<MockAudioEventHandlerRegistry> eventRegistry;
  std::shared_ptr<OfflineAudioContext> context;
  static constexpr int sampleRate = 44100;

  void SetUp() override {
    eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    context = std::make_shared<OfflineAudioContext>(
        2, 5 * sampleRate, sampleRate, eventRegistry, RuntimeRegistry{});
  }
};

TEST_F(OscillatorTest, OscillatorCanBeCreated) {
  auto osc = context->createOscillator(OscillatorOptions());
  ASSERT_NE(osc, nullptr);
}

// Regression tests for #1080 off-by-one in createBandLimitedTables silences high frequencies

TEST(PeriodicWaveBandLimiting, SineWaveAtLowFrequencyProducesAudio) {
  PeriodicWave wave(24000.0f, OscillatorType::SINE, false);
  const float phase = static_cast<float>(wave.getPeriodicWaveSize()) / 4.0f;
  float sample = wave.getSample(440.0f, phase, 440.0f * wave.getScale());
  EXPECT_GT(std::abs(sample), 0.5f);
}

TEST(PeriodicWaveBandLimiting, SineWaveAtHighFrequencyProducesAudio) {
  PeriodicWave wave(24000.0f, OscillatorType::SINE, false);
  const float phase = static_cast<float>(wave.getPeriodicWaveSize()) / 4.0f;
  float sample = wave.getSample(10000.0f, phase, 10000.0f * wave.getScale());
  EXPECT_GT(std::abs(sample), 0.1f);
}

// NOLINTEND
