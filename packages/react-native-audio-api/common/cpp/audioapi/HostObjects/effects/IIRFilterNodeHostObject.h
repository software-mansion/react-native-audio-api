#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>

#include <cstddef>
#include <memory>

namespace audioapi {
using namespace facebook;

struct IIRFilterOptions;
class BaseAudioContext;
class IIRFilterNode;

class IIRFilterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit IIRFilterNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const IIRFilterOptions &options);

  JSI_HOST_FUNCTION_DECL(getFrequencyResponse);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // Coefficient arrays (typically <= 20 floats each, 256B headroom) plus
    // the two 32-frame history buffers (xBuffers_ / yBuffers_) per channel.
    constexpr size_t kBufferLength = 32;
    return AudioNodeHostObject::getMemoryPressure() +
        /* coeff arrays */ 256 + 2 * kBufferLength * channelCount_ * sizeof(float);
  }

 private:
  IIRFilterNode *iirFilterNode_ = nullptr;
};
} // namespace audioapi
