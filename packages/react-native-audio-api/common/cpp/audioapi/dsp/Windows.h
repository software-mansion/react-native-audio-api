#pragma once

#include <audioapi/core/Constants.h>

#include <cmath>
#include <algorithm>

namespace audioapi::Windows {

// https://en.wikipedia.org/wiki/Window_function
class WindowFunction {
 public:
  explicit WindowFunction(float amplitude = 1.0f): amplitude_(amplitude) {}
  virtual void apply(float *data, int length) const = 0;

protected:
  // 1/L = amplitude
  float amplitude_;
};

//https://en.wikipedia.org/wiki/Hann_function
// https://www.sciencedirect.com/topics/engineering/hanning-window
// https://docs.scipy.org/doc//scipy-1.2.3/reference/generated/scipy.signal.windows.hann.html#scipy.signal.windows.hann
class Hann: public WindowFunction {
 public:
  explicit Hann(float amplitude = 1.0f): WindowFunction(amplitude) {};

  void apply(float *data, int length) const override;
};

// https://www.sciencedirect.com/topics/engineering/blackman-window
// https://docs.scipy.org/doc//scipy-1.2.3/reference/generated/scipy.signal.windows.blackman.html#scipy.signal.windows.blackman
class Blackman: public WindowFunction {
 public:
  explicit Blackman(float amplitude = 1.0f): WindowFunction(amplitude) {};

  void apply(float *data, int length) const override;
};


// https://en.wikipedia.org/wiki/Kaiser_window
class Kaiser: public WindowFunction {
 public:
  explicit Kaiser(float beta, float amplitude = 1.0f): WindowFunction(amplitude), beta_(beta), invB0_(1/bessel0(beta)) {};

  void apply(float *data, int length) const override;

 private:
  // beta = pi * alpha
  float beta_;
  // invB0 = 1 / I0(beta)
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
        term *= (x*x)/(4*m*m);
    }

    return result;
  }
};
// TODO: create once and reuse
// apply should be a template function
} // namespace audioapi::Windows
