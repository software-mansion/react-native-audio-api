#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/effects/PeriodicWave.h>
#include <audioapi/core/sources/OscillatorNode.h>
#include <audioapi/types/NodeOptions.h>
#include <gtest/gtest.h>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <algorithm>
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
    context = std::make_shared<OfflineAudioContext>(2, 5 * sampleRate, sampleRate, eventRegistry);
  }
};

TEST_F(OscillatorTest, OscillatorCanBeCreated) {
  auto osc = std::make_shared<OscillatorNode>(context, OscillatorOptions());
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

// Negative frequencies must:
// 1) select interpolation via |phaseIncrement| (not the signed value)
// 2) accumulate phase in double with floor-wrap (Chromium) so +/- stay
//    phase-accurate — float wrap drifts enough near zero-crossings to fail WPT.
TEST(PeriodicWaveInterpolation, NegativeFrequencyMatchesMathSine) {
  constexpr float sampleRate = 44100.0f;
  // Same absoluteThreshold as WPT osc-basic-waveform Test 0/1.
  constexpr float absThreshold = 1.8045e-5f;
  PeriodicWave wave(sampleRate, OscillatorType::SINE, false);

  const double tableSize = static_cast<double>(wave.getPeriodicWaveSize());
  const double invTableSize = 1.0 / tableSize;
  const float tableScale = wave.getScale();

  auto maxErrorFor = [&](float freqHz) {
    const float phaseInc = freqHz * tableScale;
    const double omega = 2.0 * M_PI * static_cast<double>(freqHz) / sampleRate;
    double phase = 0.0;
    float maxAbsError = 0.0f;
    for (int i = 0; i < 256; ++i) {
      const float actual = wave.getSample(freqHz, phase, phaseInc);
      const float expected = static_cast<float>(std::sin(omega * static_cast<double>(i)));
      maxAbsError = std::max(maxAbsError, std::abs(actual - expected));
      phase += static_cast<double>(phaseInc);
      phase -= std::floor(phase * invTableSize) * tableSize;
    }
    return maxAbsError;
  };

  const float errPos = maxErrorFor(100.0f);
  const float errNeg = maxErrorFor(-100.0f);
  EXPECT_LT(errPos, absThreshold) << "positive 100 Hz max abs error " << errPos;
  EXPECT_LT(errNeg, absThreshold) << "negative 100 Hz max abs error " << errNeg;
  EXPECT_NEAR(errPos, errNeg, 5e-6f) << "pos/neg should stay similarly accurate";
}

// NOLINTEND
