/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <audioapi/core/utils/Constants.h>
#include <test/src/biquad/BiquadFilterChromium.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <span>
#include <vector>

namespace audioapi {

// Compute 10^x = exp(x*log(10))
static double pow10(double x) {
  return std::exp(x * 2.30258509299404568402);
}

void getFrequencyResponse(
    const BiquadCoefficients &coeffs,
    std::span<const float> frequency,
    std::span<float> mag_response,
    std::span<float> phase_response,
    float nyquistFrequency) {
  assert(!frequency.empty());
  assert(!mag_response.empty());
  assert(!phase_response.empty());

  std::vector<float> normalizedFreq(frequency.size());
  for (size_t i = 0; i < frequency.size(); ++i) {
    normalizedFreq[i] = frequency[i] / nyquistFrequency;
  }

  // Evaluate the Z-transform of the filter at given normalized
  // frequency from 0 to 1.  (1 corresponds to the Nyquist
  // frequency.)
  //
  // The z-transform of the filter is
  //
  // H(z) = (b0 + b1*z^(-1) + b2*z^(-2))/(1 + a1*z^(-1) + a2*z^(-2))
  //
  // Evaluate as
  //
  // b0 + (b1 + b2*z1)*z1
  // --------------------
  // 1 + (a1 + a2*z1)*z1
  //
  // with z1 = 1/z and z = exp(j*pi*frequency). Hence z1 = exp(-j*pi*frequency)

  double b0 = coeffs.b0;
  double b1 = coeffs.b1;
  double b2 = coeffs.b2;
  double a1 = coeffs.a1;
  double a2 = coeffs.a2;

  for (size_t k = 0; k < normalizedFreq.size(); ++k) {
    if (normalizedFreq[k] < 0.0 || normalizedFreq[k] > 1.0) {
      // Out-of-bounds frequencies should return NaN.
      mag_response[k] = std::nanf("");
      phase_response[k] = std::nanf("");
    } else {
      double omega = -PI * normalizedFreq[k];
      std::complex<double> z = std::complex<double>(std::cos(omega), std::sin(omega));
      std::complex<double> numerator = b0 + (b1 + b2 * z) * z;
      std::complex<double> denominator = std::complex<double>(1, 0) + (a1 + a2 * z) * z;
      std::complex<double> response = numerator / denominator;
      mag_response[k] = static_cast<float>(abs(response));
      phase_response[k] = static_cast<float>(std::atan2(imag(response), real(response)));
    }
  }
}

BiquadCoefficients
normalizeCoefficients(double b0, double b1, double b2, double a0, double a1, double a2) {
  return BiquadCoefficients{b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

BiquadCoefficients calculateLowpassCoefficients(double cutoff, double resonance) {
  // Limit cutoff to 0 to 1.
  cutoff = std::clamp(cutoff, 0.0, 1.0);

  if (cutoff == 1) {
    // When cutoff is 1, the z-transform is 1.
    return normalizeCoefficients(1, 0, 0, 1, 0, 0);
  } else if (cutoff > 0) {
    // Compute biquad coefficients for lowpass filter

    resonance = pow10(resonance / 20);

    double theta = kPiDouble * cutoff;
    double alpha = std::sin(theta) / (2 * resonance);
    double cosw = std::cos(theta);
    double beta = (1 - cosw) / 2;

    double b0 = beta;
    double b1 = 2 * beta;
    double b2 = beta;

    double a0 = 1 + alpha;
    double a1 = -2 * cosw;
    double a2 = 1 - alpha;

    return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
  } else {
    // When cutoff is zero, nothing gets through the filter, so set
    // coefficients up correctly.
    return normalizeCoefficients(0, 0, 0, 1, 0, 0);
  }
}

BiquadCoefficients calculateHighpassCoefficients(double cutoff, double resonance) {
  // Limit cutoff to 0 to 1.
  cutoff = std::clamp(cutoff, 0.0, 1.0);

  if (cutoff == 1) {
    // The z-transform is 0.
    return normalizeCoefficients(0, 0, 0, 1, 0, 0);
  } else if (cutoff > 0) {
    // Compute biquad coefficients for highpass filter

    resonance = pow10(resonance / 20);
    double theta = kPiDouble * cutoff;
    double alpha = std::sin(theta) / (2 * resonance);
    double cosw = std::cos(theta);
    double beta = (1 + cosw) / 2;

    double b0 = beta;
    double b1 = -2 * beta;
    double b2 = beta;

    double a0 = 1 + alpha;
    double a1 = -2 * cosw;
    double a2 = 1 - alpha;

    return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
  } else {
    // When cutoff is zero, we need to be careful because the above
    // gives a quadratic divided by the same quadratic, with poles
    // and zeros on the unit circle in the same place. When cutoff
    // is zero, the z-transform is 1.
    return normalizeCoefficients(1, 0, 0, 1, 0, 0);
  }
}

BiquadCoefficients calculateLowshelfCoefficients(double frequency, double db_gain) {
  // Clip frequencies to between 0 and 1, inclusive.
  frequency = std::clamp(frequency, 0.0, 1.0);

  double a = pow10(db_gain / 40);

  if (frequency == 1) {
    // The z-transform is a constant gain.
    return normalizeCoefficients(a * a, 0, 0, 1, 0, 0);
  } else if (frequency > 0) {
    double w0 = kPiDouble * frequency;
    double s = 1; // filter slope (1 is max value)
    double alpha = 0.5 * std::sin(w0) * sqrt((a + 1 / a) * (1 / s - 1) + 2);
    double k = std::cos(w0);
    double k2 = 2 * sqrt(a) * alpha;
    double a_plus_one = a + 1;
    double a_minus_one = a - 1;

    double b0 = a * (a_plus_one - a_minus_one * k + k2);
    double b1 = 2 * a * (a_minus_one - a_plus_one * k);
    double b2 = a * (a_plus_one - a_minus_one * k - k2);
    double a0 = a_plus_one + a_minus_one * k + k2;
    double a1 = -2 * (a_minus_one + a_plus_one * k);
    double a2 = a_plus_one + a_minus_one * k - k2;

    return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
  } else {
    // When frequency is 0, the z-transform is 1.
    return normalizeCoefficients(1, 0, 0, 1, 0, 0);
  }
}

BiquadCoefficients calculateHighshelfCoefficients(double frequency, double db_gain) {
  // Clip frequencies to between 0 and 1, inclusive.
  frequency = std::clamp(frequency, 0.0, 1.0);

  double a = pow10(db_gain / 40);

  if (frequency == 1) {
    // The z-transform is 1.
    return normalizeCoefficients(1, 0, 0, 1, 0, 0);
  } else if (frequency > 0) {
    double w0 = kPiDouble * frequency;
    double s = 1; // filter slope (1 is max value)
    double alpha = 0.5 * std::sin(w0) * sqrt((a + 1 / a) * (1 / s - 1) + 2);
    double k = std::cos(w0);
    double k2 = 2 * sqrt(a) * alpha;
    double a_plus_one = a + 1;
    double a_minus_one = a - 1;

    double b0 = a * (a_plus_one + a_minus_one * k + k2);
    double b1 = -2 * a * (a_minus_one + a_plus_one * k);
    double b2 = a * (a_plus_one + a_minus_one * k - k2);
    double a0 = a_plus_one - a_minus_one * k + k2;
    double a1 = 2 * (a_minus_one - a_plus_one * k);
    double a2 = a_plus_one - a_minus_one * k - k2;

    return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
  } else {
    // When frequency = 0, the filter is just a gain, A^2.
    return normalizeCoefficients(a * a, 0, 0, 1, 0, 0);
  }
}

BiquadCoefficients calculatePeakingCoefficients(double frequency, double q, double db_gain) {
  // Clip frequencies to between 0 and 1, inclusive.
  frequency = std::clamp(frequency, 0.0, 1.0);

  // Don't let Q go negative, which causes an unstable filter.
  q = std::max(0.0, q);

  double a = pow10(db_gain / 40);

  if (frequency > 0 && frequency < 1) {
    if (q > 0) {
      double w0 = kPiDouble * frequency;
      double alpha = std::sin(w0) / (2 * q);
      double k = std::cos(w0);

      double b0 = 1 + alpha * a;
      double b1 = -2 * k;
      double b2 = 1 - alpha * a;
      double a0 = 1 + alpha / a;
      double a1 = -2 * k;
      double a2 = 1 - alpha / a;

      return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
    } else {
      // When Q = 0, the above formulas have problems. If we look at
      // the z-transform, we can see that the limit as Q->0 is A^2, so
      // set the filter that way.
      return normalizeCoefficients(a * a, 0, 0, 1, 0, 0);
    }
  } else {
    // When frequency is 0 or 1, the z-transform is 1.
    return normalizeCoefficients(1, 0, 0, 1, 0, 0);
  }
}

BiquadCoefficients calculateAllpassCoefficients(double frequency, double q) {
  // Clip frequencies to between 0 and 1, inclusive.
  frequency = std::clamp(frequency, 0.0, 1.0);

  // Don't let Q go negative, which causes an unstable filter.
  q = std::max(0.0, q);

  if (frequency > 0 && frequency < 1) {
    if (q > 0) {
      double w0 = kPiDouble * frequency;
      double alpha = std::sin(w0) / (2 * q);
      double k = std::cos(w0);

      double b0 = 1 - alpha;
      double b1 = -2 * k;
      double b2 = 1 + alpha;
      double a0 = 1 + alpha;
      double a1 = -2 * k;
      double a2 = 1 - alpha;

      return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
    } else {
      // When Q = 0, the above formulas have problems. If we look at
      // the z-transform, we can see that the limit as Q->0 is -1, so
      // set the filter that way.
      return normalizeCoefficients(-1, 0, 0, 1, 0, 0);
    }
  } else {
    // When frequency is 0 or 1, the z-transform is 1.
    return normalizeCoefficients(1, 0, 0, 1, 0, 0);
  }
}

BiquadCoefficients calculateNotchCoefficients(double frequency, double q) {
  // Clip frequencies to between 0 and 1, inclusive.
  frequency = std::clamp(frequency, 0.0, 1.0);

  // Don't let Q go negative, which causes an unstable filter.
  q = std::max(0.0, q);

  if (frequency > 0 && frequency < 1) {
    if (q > 0) {
      double w0 = kPiDouble * frequency;
      double alpha = std::sin(w0) / (2 * q);
      double k = std::cos(w0);

      double b0 = 1;
      double b1 = -2 * k;
      double b2 = 1;
      double a0 = 1 + alpha;
      double a1 = -2 * k;
      double a2 = 1 - alpha;

      return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
    } else {
      // When Q = 0, the above formulas have problems. If we look at
      // the z-transform, we can see that the limit as Q->0 is 0, so
      // set the filter that way.
      return normalizeCoefficients(0, 0, 0, 1, 0, 0);
    }
  } else {
    // When frequency is 0 or 1, the z-transform is 1.
    return normalizeCoefficients(1, 0, 0, 1, 0, 0);
  }
}

BiquadCoefficients calculateBandpassCoefficients(double frequency, double q) {
  // No negative frequencies allowed.
  frequency = std::max(0.0, frequency);

  // Don't let Q go negative, which causes an unstable filter.
  q = std::max(0.0, q);

  if (frequency > 0 && frequency < 1) {
    double w0 = kPiDouble * frequency;
    if (q > 0) {
      double alpha = std::sin(w0) / (2 * q);
      double k = std::cos(w0);

      double b0 = alpha;
      double b1 = 0;
      double b2 = -alpha;
      double a0 = 1 + alpha;
      double a1 = -2 * k;
      double a2 = 1 - alpha;

      return normalizeCoefficients(b0, b1, b2, a0, a1, a2);
    } else {
      // When Q = 0, the above formulas have problems. If we look at
      // the z-transform, we can see that the limit as Q->0 is 1, so
      // set the filter that way.
      return normalizeCoefficients(1, 0, 0, 1, 0, 0);
    }
  } else {
    // When the cutoff is zero, the z-transform approaches 0, if Q
    // > 0. When both Q and cutoff are zero, the z-transform is
    // pretty much undefined. What should we do in this case?
    // For now, just make the filter 0. When the cutoff is 1, the
    // z-transform also approaches 0.
    return normalizeCoefficients(0, 0, 0, 1, 0, 0);
  }
}
} // namespace audioapi
