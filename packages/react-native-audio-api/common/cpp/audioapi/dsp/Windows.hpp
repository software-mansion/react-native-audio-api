#pragma once

#include <algorithm>
#include <cmath>
#include <span>

namespace audioapi::dsp {

// https://en.wikipedia.org/wiki/Window_function
// https://personalpages.hs-kempten.de/~vollratj/InEl/pdf/Window%20function%20-%20Wikipedia.pdf
class WindowFunction {
 public:
  explicit WindowFunction(float amplitude = 1.0f) : amplitude_(amplitude) {}

  virtual void apply(std::span<float> data) const noexcept = 0;
  // forces STFT perfect-reconstruction (WOLA) on an existing window, for a given STFT interval.
  static void forcePerfectReconstruction(std::span<float> data, int interval) {
    int windowLength = static_cast<int>(data.size());

    for (int i = 0; i < interval; ++i) {
      float sum2 = 0;

      for (int index = i; index < windowLength; index += interval) {
        sum2 += data[index] * data[index];
      }

      float factor = 1 / std::sqrt(sum2);

      for (int index = i; index < windowLength; index += interval) {
        data[index] *= factor;
      }
    }
  }

 protected:
  // 1/L = amplitude
  float amplitude_;
};

//https://en.wikipedia.org/wiki/Hann_function
// https://www.sciencedirect.com/topics/engineering/hanning-window
// https://docs.scipy.org/doc//scipy-1.2.3/reference/generated/scipy.signal.windows.hann.html#scipy.signal.windows.hann
class Hann : public WindowFunction {
 public:
  explicit Hann(float amplitude = 1.0f) : WindowFunction(amplitude) {}

  void apply(std::span<float> data) const noexcept override {
    const size_t size = data.size();
    if (size < 2) {
      return;
    }

    const float invSizeMinusOne = 1.0f / static_cast<float>(size - 1);
    const float constantPart = 2.0f * std::numbers::pi_v<float> * invSizeMinusOne;

    for (size_t i = 0; i < size; ++i) {
      float window = 0.5f * (1.0f - std::cos(constantPart * i));
      data[i] = window * amplitude_;
    }
  }
};

// https://www.sciencedirect.com/topics/engineering/blackman-window
// https://docs.scipy.org/doc//scipy-1.2.3/reference/generated/scipy.signal.windows.blackman.html#scipy.signal.windows.blackman
class Blackman : public WindowFunction {
 public:
  explicit Blackman(float amplitude = 1.0f) : WindowFunction(amplitude) {}

  void apply(std::span<float> data) const noexcept override {
    const size_t size = data.size();
    if (size < 2) {
      return;
    }

    const float invSizeMinusOne = 1.0f / static_cast<float>(size - 1);
    const float alpha = 2.0f * std::numbers::pi_v<float> * invSizeMinusOne;

    for (size_t i = 0; i < size; ++i) {
      const float phase = alpha * i;
      // 4*PI*x is just 2 * (2*PI*x)
      const float window = 0.42f - 0.50f * std::cos(phase) + 0.08f * std::cos(2.0f * phase);
      data[i] = window * amplitude_;
    }
  }
};

// https://en.wikipedia.org/wiki/Kaiser_window
class Kaiser : public WindowFunction {
 public:
  explicit Kaiser(float beta, float amplitude = 1.0f)
      : WindowFunction(amplitude), beta_(beta), invB0_(1.0f / bessel0(beta)) {}

  static Kaiser
  withBandwidth(float bandwidth, bool heuristicOptimal = false, float amplitude = 1.0f) {
    return Kaiser(bandwidthToBeta(bandwidth, heuristicOptimal), amplitude);
  }

  void apply(std::span<float> data) const noexcept override {
    const size_t size = data.size();
    if (size == 0) {
      return;
    }

    const float invSize = 1.0f / static_cast<float>(size);
    const float commonScale = invB0_ * amplitude_;

    for (size_t i = 0; i < size; ++i) {
      // Optimized 'r' calculation: (2i+1)/size - 1
      const float r = (static_cast<float>(2 * i + 1) * invSize) - 1.0f;
      const float arg = std::sqrt(std::max(0.0f, 1.0f - r * r));

      data[i] = bessel0(beta_ * arg) * commonScale;
    }
  }

 private:
  // beta = pi * alpha
  // invB0 = 1 / I0(beta)
  float beta_;
  float invB0_;

  // https://en.wikipedia.org/wiki/Bessel_function#Modified_Bessel_functions:_I%CE%B1,_K%CE%B1
  static inline float bessel0(float x) {
    const double significanceLimit = 1e-4;
    auto result = 0.0f;
    auto term = 1.0f;
    auto m = 1.0f;
    while (term > significanceLimit) {
      result += term;
      ++m;
      term *= (x * x) / (4 * m * m);
    }

    return result;
  }
  inline static float bandwidthToBeta(float bandwidth, bool heuristicOptimal = false) {
    if (heuristicOptimal) { // Heuristic based on numerical search
      return bandwidth + 8.0f / (bandwidth + 3.0f) * (bandwidth + 3.0f) +
          0.25f * std::max(3.0f - bandwidth, 0.0f);
    }

    bandwidth = std::max(bandwidth, 2.0f);
    auto alpha = std::sqrt(bandwidth * bandwidth * 0.25f - 1.0f);
    return alpha * PI;
  }
};

// https://www.recordingblogs.com/wiki/gaussian-window
class ApproximateConfinedGaussian : public WindowFunction {
 public:
  explicit ApproximateConfinedGaussian(float sigma, float amplitude = 1.0f)
      : WindowFunction(amplitude), gaussianFactor_(0.0625f / (sigma * sigma)) {}

  static ApproximateConfinedGaussian withBandwidth(float bandwidth, float amplitude = 1.0f) {
    return ApproximateConfinedGaussian(bandwidthToSigma(bandwidth), amplitude);
  }

  void apply(std::span<float> data) const noexcept override {
    const size_t size = data.size();
    if (size == 0)
      return;

    const float g1 = getGaussian(1.0f);
    const float g3 = getGaussian(3.0f);
    const float g_1 = getGaussian(-1.0f);
    const float g2 = getGaussian(2.0f);

    const float offsetScale = g1 / (g3 + g_1);
    const float norm = 1.0f / (g1 - 2.0f * offsetScale * g2);

    const float invSize = 1.0f / static_cast<float>(size);
    const float totalAmplitude = norm * amplitude_;

    for (size_t i = 0; i < size; ++i) {
      const float r = (static_cast<float>(2 * i + 1) * invSize) - 1.0f;

      const float gR = getGaussian(r);
      const float gRMinus2 = getGaussian(r - 2.0f);
      const float gRPlus2 = getGaussian(r + 2.0f);

      data[i] = totalAmplitude * (gR - offsetScale * (gRMinus2 + gRPlus2));
    }
  }

 private:
  float gaussianFactor_;

  inline static float bandwidthToSigma(float bandwidth) noexcept {
    return 0.3f / std::sqrt(bandwidth);
  }

  [[nodiscard]] inline float getGaussian(float x) const noexcept {
    return std::exp(-x * x * gaussianFactor_);
  }
};
} // namespace audioapi::dsp
