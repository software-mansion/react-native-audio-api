#pragma once

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct BaseAudioBufferSourceOptions;
class BaseAudioContext;
class AudioBufferQueueSourceNode;

class AudioBufferQueueSourceNodeHostObject : public AudioBufferBaseSourceNodeHostObject {
 public:
  explicit AudioBufferQueueSourceNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const BaseAudioBufferSourceOptions &options);

  ~AudioBufferQueueSourceNodeHostObject() override;

  JSI_PROPERTY_SETTER_DECL(onBufferEnded);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(pause);
  JSI_HOST_FUNCTION_DECL(enqueueBuffer);
  JSI_HOST_FUNCTION_DECL(dequeueBuffer);
  JSI_HOST_FUNCTION_DECL(clearBuffers);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // Same as AudioBufferSourceNode: playbackRate + detune. Enqueued buffers
    // are individually tracked via their own AudioBufferHostObject pressure.
    return AudioNodeHostObject::getMemoryPressure() + 2 * kAudioParamBytes;
  }

 protected:
  AudioBufferQueueSourceNode *bufferQueueSourceNode_ = nullptr;

  size_t bufferId_ = 0;
  bool stretchHasBeenInit_ = false;
  bool channelCountSet_ = false;
};

} // namespace audioapi
