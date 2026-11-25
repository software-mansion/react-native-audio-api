#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/Constants.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class BaseAudioContext;

class RecorderAdapterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit RecorderAdapterNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioScheduledSourceNodeOptions &options = AudioScheduledSourceNodeOptions());

  /// The node's AudioConsumer, which JS `AudioRecorder.connect` adds to the recorder.
  JSI_PROPERTY_GETTER_DECL(consumer);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // Small resampling ring buffer (~RQ * ratio frames * channels). At most a
    // few KB — we're generous by one order of magnitude to cover worst-case
    // mic/context sample-rate ratios.
    return AudioNodeHostObject::getMemoryPressure() +
        4 * RENDER_QUANTUM_SIZE * channelCount_ * sizeof(float);
  }

 private:
  std::shared_ptr<RecorderAdapterConsumer> consumer_;
};

} // namespace audioapi
