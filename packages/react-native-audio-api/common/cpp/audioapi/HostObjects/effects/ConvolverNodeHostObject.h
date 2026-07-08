#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {
using namespace facebook;

struct ConvolverOptions;
class BaseAudioContext;
class ConvolverNode;

class ConvolverNodeHostObject : public AudioNodeHostObject {
 public:
  explicit ConvolverNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConvolverOptions &options);
  JSI_PROPERTY_GETTER_DECL(normalize);
  JSI_PROPERTY_SETTER_DECL(normalize);
  JSI_HOST_FUNCTION_DECL(setBuffer);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // Approximate IR-driven allocations: IR copy + partitioned convolver
    // state + internal/intermediate DSP buffers ≈ 3 * IR bytes.
    return AudioNodeHostObject::getMemoryPressure() + 3 * irBytes_;
  }

 private:
  ConvolverNode *convolverNode_ = nullptr;

  bool normalize_;
  size_t irBytes_ = 0;
  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);
};
} // namespace audioapi
