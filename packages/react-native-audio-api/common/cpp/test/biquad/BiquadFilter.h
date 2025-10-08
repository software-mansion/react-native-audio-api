#pragma once

namespace audioapi {
struct BiquadCoefficients {
  float b0;
  float b1;
  float b2;
  float a1;
  float a2;
};

BiquadCoefficients normalizeCoefficients(float b0, float b1, float b2, float a0, float a1, float a2);

BiquadCoefficients calculateLowpassCoefficients(float cutoff, float Q);
BiquadCoefficients calculateHighpassCoefficients(float cutoff, float Q);
BiquadCoefficients calculateBandpassCoefficients(float frequency, float Q);
BiquadCoefficients calculateNotchCoefficients(float frequency, float Q);
BiquadCoefficients calculateAllpassCoefficients(float frequency, float Q);
BiquadCoefficients calculatePeakingCoefficients(float frequency, float Q, float db_gain);
BiquadCoefficients calculateLowshelfCoefficients(float frequency, float db_gain);
BiquadCoefficients calculateHighshelfCoefficients(float frequency, float db_gain);

} // namespace audioapi
