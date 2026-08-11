#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/types/OverSampleType.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct WaveShaperOptions;
class BaseAudioContext;
class WaveShaperNode;

class WaveShaperNodeHostObject : public AudioNodeHostObject {
 public:
  explicit WaveShaperNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const WaveShaperOptions &options);

  JSI_PROPERTY_GETTER_DECL(oversample);

  JSI_PROPERTY_SETTER_DECL(oversample);
  JSI_HOST_FUNCTION_DECL(setCurve);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // 2x and 4x oversample scratch buffers: (RQ*2 + RQ*4) * 2 (I/O) * float.
    // Resamplers carry roughly the same amount of internal state again.
    return AudioNodeHostObject::getMemoryPressure() + 12 * RENDER_QUANTUM_SIZE * sizeof(float) +
        curveMemoryPressure_;
  }

 private:
  void scheduleCurveUpdate(const std::shared_ptr<AudioArray> &curve);

  WaveShaperNode *const waveShaperNode_;

  OverSampleType oversample_;
  size_t curveMemoryPressure_{0};
};
} // namespace audioapi
