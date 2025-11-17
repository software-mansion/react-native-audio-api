#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/Windows.h>
#include <algorithm>

namespace audioapi::dsp {

void WindowFunction::forcePerfectReconstruction(float *data, int windowLength, int interval) {
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

void Hann::apply(float *data, int size) const {
  for (int i = 0; i < size; ++i) {
    auto x = static_cast<float>(i) / static_cast<float>(size - 1);
    auto window = 0.5f - 0.5f * cos(2 * PI * x);
    data[i] = window * amplitude_;
  }
}

void Blackman::apply(float *data, int size) const {
  for (int i = 0; i < size; ++i) {
    auto x = static_cast<float>(i) / static_cast<float>(size - 1);
    auto window = 0.42f - 0.5f * cos(2 * PI * x) + 0.08f * cos(4 * PI * x);
    data[i] = window * amplitude_;
  }
}

void Kaiser::apply(float *data, int size) const {
  for (int i = 0; i < size; ++i) {
    auto r = static_cast<float>(2 * i + 1) / static_cast<float>(size) - 1.0f;
    auto arg = std::sqrt(1 - r * r);
    data[i] = bessel0(beta_ * arg) * invB0_ * amplitude_;
  }
}

float Kaiser::bandwidthToBeta(float bandwidth, bool heuristicOptimal) {
  if (heuristicOptimal) { // Heuristic based on numerical search
    return bandwidth + 8.0f / (bandwidth + 3.0f) * (bandwidth + 3.0f) +
        0.25f * std::max(3.0f - bandwidth, 0.0f);
  }

  bandwidth = std::max(bandwidth, 2.0f);
  auto alpha = std::sqrt(bandwidth * bandwidth * 0.25f - 1.0f);
  return alpha * PI;
}

void ApproximateConfinedGaussian::apply(float *data, int size) const {
  auto offsetScale = getGaussian(1.0f) / (getGaussian(3.0f) + getGaussian(-1.0f));
  auto norm = 1 / (getGaussian(1.0f) - 2 * offsetScale * getGaussian(2.0f));
  for (int i = 0; i < size; ++i) {
    auto r = static_cast<float>(2 * i + 1) / static_cast<float>(size) - 1.0f;
    data[i] = norm * (getGaussian(r) - offsetScale * (getGaussian(r - 2) + getGaussian(r + 2)));
  }
}

float ApproximateConfinedGaussian::bandwidthToSigma(float bandwidth) {
  return 0.3f / std::sqrt(bandwidth);
}

float ApproximateConfinedGaussian::getGaussian(float x) const {
  return std::exp(-x * x * gaussianFactor_);
}

} // namespace audioapi::dsp
