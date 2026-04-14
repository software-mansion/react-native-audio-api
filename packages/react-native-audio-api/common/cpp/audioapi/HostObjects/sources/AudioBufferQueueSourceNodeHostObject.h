#pragma once

#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

struct BaseAudioBufferSourceOptions;
class BaseAudioContext;

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

 protected:
  size_t bufferId_ = 0;
  uint64_t onBufferEndedCallbackId_ = 0;
  bool stretchHasBeenInit_ = false;

  void setOnBufferEndedCallbackId(uint64_t callbackId);
};

} // namespace audioapi
