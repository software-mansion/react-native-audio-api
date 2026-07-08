#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/sources/RecorderAdapterNode.h>
#include <audioapi/core/utils/graph/Graph.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioRecorderHostObject;

class RecorderAdapterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit RecorderAdapterNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioScheduledSourceNodeOptions &options = AudioScheduledSourceNodeOptions())
      : AudioNodeHostObject(
            context->getGraph(),
            std::make_unique<RecorderAdapterNode>(context),
            options) {}

  [[nodiscard]] size_t getMemoryPressure() const override {
    // Small resampling ring buffer (~RQ * ratio frames * channels). At most a
    // few KB — we're generous by one order of magnitude to cover worst-case
    // mic/context sample-rate ratios.
    return AudioNodeHostObject::getMemoryPressure() +
        4 * RENDER_QUANTUM_SIZE * channelCount_ * sizeof(float);
  }

 private:
  friend class AudioRecorderHostObject;
};

} // namespace audioapi
