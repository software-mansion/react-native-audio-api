#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct AnalyserOptions;
class BaseAudioContext;
class AnalyserNode;

class AnalyserNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AnalyserNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AnalyserOptions &options);

  JSI_PROPERTY_GETTER_DECL(fftSize);
  JSI_PROPERTY_GETTER_DECL(minDecibels);
  JSI_PROPERTY_GETTER_DECL(maxDecibels);
  JSI_PROPERTY_GETTER_DECL(smoothingTimeConstant);

  JSI_PROPERTY_SETTER_DECL(fftSize);
  JSI_PROPERTY_SETTER_DECL(minDecibels);
  JSI_PROPERTY_SETTER_DECL(maxDecibels);
  JSI_PROPERTY_SETTER_DECL(smoothingTimeConstant);

  JSI_HOST_FUNCTION_DECL(getFloatFrequencyData);
  JSI_HOST_FUNCTION_DECL(getByteFrequencyData);
  JSI_HOST_FUNCTION_DECL(getFloatTimeDomainData);
  JSI_HOST_FUNCTION_DECL(getByteTimeDomainData);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // Analyser sizes dominate everything else:
    // - CircularDSPAudioArray(MAX_FFT_SIZE * 2) input ring
    // - TripleBuffer<AnalysisFrame>(MAX_FFT_SIZE) = 3 * MAX_FFT * float
    // - FFT scratch + window + smoothed magnitudes (~MAX_FFT * float each,
    //   conservatively 3x).
    return AudioNodeHostObject::getMemoryPressure() + 2 * MAX_FFT_SIZE * sizeof(float) +
        3 * MAX_FFT_SIZE * sizeof(float) + 3 * MAX_FFT_SIZE * sizeof(float);
  }

 private:
  AnalyserNode *analyserNode_ = nullptr;
};

} // namespace audioapi
