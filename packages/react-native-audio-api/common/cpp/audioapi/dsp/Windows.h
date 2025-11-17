#pragma once

#include <cmath>

namespace audioapi::dsp {

// https://en.wikipedia.org/wiki/Window_function
// https://personalpages.hs-kempten.de/~vollratj/InEl/pdf/Window%20function%20-%20Wikipedia.pdf
class WindowFunction {
 public:
  explicit WindowFunction(float amplitude = 1.0f) : amplitude_(amplitude) {}

  virtual void apply(float *data, int size) const = 0;
  // forces STFT perfect-reconstruction (WOLA) on an existing window, for a given STFT interval.
  static void forcePerfectReconstruction(float *data, int windowLength, int interval);

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

  void apply(float *data, int size) const override;
};

// https://www.sciencedirect.com/topics/engineering/blackman-window
// https://docs.scipy.org/doc//scipy-1.2.3/reference/generated/scipy.signal.windows.blackman.html#scipy.signal.windows.blackman
class Blackman : public WindowFunction {
 public:
  explicit Blackman(float amplitude = 1.0f) : WindowFunction(amplitude) {}

  void apply(float *data, int size) const override;
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

  void apply(float *data, int size) const override;

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
  static float bandwidthToBeta(float bandwidth, bool heuristicOptimal = false);
};

// https://www.recordingblogs.com/wiki/gaussian-window
class ApproximateConfinedGaussian : public WindowFunction {
 public:
  explicit ApproximateConfinedGaussian(float sigma, float amplitude = 1.0f)
      : WindowFunction(amplitude), gaussianFactor_(0.0625f / (sigma * sigma)) {}

  static ApproximateConfinedGaussian withBandwidth(float bandwidth, float amplitude = 1.0f) {
    return ApproximateConfinedGaussian(bandwidthToSigma(bandwidth), amplitude);
  }

  void apply(float *data, int size) const override;

 private:
  float gaussianFactor_;

  static float bandwidthToSigma(float bandwidth);

  [[nodiscard]] float getGaussian(float x) const;
};
} // namespace audioapi::dsp
