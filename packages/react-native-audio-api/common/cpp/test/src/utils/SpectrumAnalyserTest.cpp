#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/SpectrumAnalyser.h>
#include <gtest/gtest.h>

#include <cmath>

using namespace audioapi;
using audioapi::dsp::SpectrumAnalyser;

// NOLINTBEGIN

namespace {

constexpr float kSampleRate = 44100.0f;
constexpr int kFFTSize = 1024;

// Fills `timeDomain` with a sine wave whose frequency lands exactly on FFT bin
// `binIndex` (i.e. frequency = binIndex * kSampleRate / kFFTSize), so the
// expected peak bin is deterministic modulo windowing spread.
void fillSineAtBin(DSPAudioArray &timeDomain, int binIndex) {
  const float frequency = static_cast<float>(binIndex) * kSampleRate / static_cast<float>(kFFTSize);
  auto data = timeDomain.span();

  for (size_t i = 0; i < timeDomain.getSize(); ++i) {
    data[i] = std::sin(2.0f * PI * frequency * static_cast<float>(i) / kSampleRate);
  }
}

int argmax(const DSPAudioArray &magnitudes) {
  auto span = magnitudes.span();
  int best = 0;
  for (size_t i = 1; i < span.size(); ++i) {
    if (span[i] > span[best]) {
      best = static_cast<int>(i);
    }
  }
  return best;
}

} // namespace

class SpectrumAnalyserTest : public ::testing::Test {
 protected:
  SpectrumAnalyser analyser_{kFFTSize};
};

TEST_F(SpectrumAnalyserTest, ExposesConfiguredFFTSize) {
  EXPECT_EQ(analyser_.getFFTSize(), kFFTSize);
}

TEST_F(SpectrumAnalyserTest, MagnitudeDataHasHalfFFTSizeBins) {
  EXPECT_EQ(analyser_.getMagnitudeData().getSize(), static_cast<size_t>(kFFTSize / 2));
}

TEST_F(SpectrumAnalyserTest, StartsWithZeroedMagnitudes) {
  auto span = analyser_.getMagnitudeData().span();
  for (float value : span) {
    EXPECT_FLOAT_EQ(value, 0.0f);
  }
}

TEST_F(SpectrumAnalyserTest, AnalyzeFindsPeakAtExpectedBin) {
  constexpr int kBinIndex = 100;
  DSPAudioArray timeDomain(kFFTSize);
  fillSineAtBin(timeDomain, kBinIndex);

  analyser_.analyze(timeDomain, /*smoothingTimeConstant=*/0.0f);

  EXPECT_NEAR(argmax(analyser_.getMagnitudeData()), kBinIndex, 1);
}

TEST_F(SpectrumAnalyserTest, ZeroSmoothingFullyReplacesMagnitude) {
  constexpr int kBinIndex = 100;
  DSPAudioArray timeDomain(kFFTSize);
  fillSineAtBin(timeDomain, kBinIndex);

  analyser_.analyze(timeDomain, /*smoothingTimeConstant=*/0.0f);
  const float firstPass = analyser_.getMagnitudeData().span()[kBinIndex];

  analyser_.analyze(timeDomain, /*smoothingTimeConstant=*/0.0f);
  const float secondPass = analyser_.getMagnitudeData().span()[kBinIndex];

  EXPECT_NEAR(firstPass, secondPass, 1e-5f);
  EXPECT_GT(firstPass, 0.0f);
}

TEST_F(SpectrumAnalyserTest, SmoothingBlendsTowardNewMagnitudeGradually) {
  constexpr int kBinIndex = 100;
  constexpr float kSmoothingTimeConstant = 0.9f;

  DSPAudioArray silence(kFFTSize);
  silence.zero();

  DSPAudioArray tone(kFFTSize);
  fillSineAtBin(tone, kBinIndex);

  // Establish a zero baseline, then blend in the tone with heavy smoothing:
  // the new value should be (1 - smoothing) * instantaneous, i.e. far below
  // the fully-replaced (unsmoothed) magnitude but still positive.
  analyser_.analyze(silence, /*smoothingTimeConstant=*/0.0f);
  analyser_.analyze(tone, kSmoothingTimeConstant);
  const float smoothed = analyser_.getMagnitudeData().span()[kBinIndex];

  SpectrumAnalyser unsmoothed(kFFTSize);
  unsmoothed.analyze(tone, /*smoothingTimeConstant=*/0.0f);
  const float instantaneous = unsmoothed.getMagnitudeData().span()[kBinIndex];

  EXPECT_GT(smoothed, 0.0f);
  EXPECT_LT(smoothed, instantaneous);
  EXPECT_NEAR(smoothed, (1.0f - kSmoothingTimeConstant) * instantaneous, 1e-5f);
}

TEST_F(SpectrumAnalyserTest, SetFFTSizeReallocatesAndResetsMagnitudes) {
  DSPAudioArray timeDomain(kFFTSize);
  fillSineAtBin(timeDomain, 100);
  analyser_.analyze(timeDomain, 0.0f);
  ASSERT_GT(analyser_.getMagnitudeData().span()[100], 0.0f);

  constexpr int kNewFFTSize = 2048;
  analyser_.setFFTSize(kNewFFTSize);

  EXPECT_EQ(analyser_.getFFTSize(), kNewFFTSize);
  EXPECT_EQ(analyser_.getMagnitudeData().getSize(), static_cast<size_t>(kNewFFTSize / 2));
  for (float value : analyser_.getMagnitudeData().span()) {
    EXPECT_FLOAT_EQ(value, 0.0f);
  }
}

TEST_F(SpectrumAnalyserTest, SetFFTSizeIsNoOpWhenUnchanged) {
  DSPAudioArray timeDomain(kFFTSize);
  fillSineAtBin(timeDomain, 100);
  analyser_.analyze(timeDomain, 0.0f);
  const float before = analyser_.getMagnitudeData().span()[100];

  analyser_.setFFTSize(kFFTSize);

  EXPECT_FLOAT_EQ(analyser_.getMagnitudeData().span()[100], before);
}

// NOLINTEND
