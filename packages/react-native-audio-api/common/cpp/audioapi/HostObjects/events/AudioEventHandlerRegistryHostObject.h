#pragma once

#include <audioapi/jsi/HostObject.h>

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class IAudioEventHandlerRegistry;

class AudioEventHandlerRegistryHostObject : public HostObject {
 public:
  explicit AudioEventHandlerRegistryHostObject(
      const std::shared_ptr<IAudioEventHandlerRegistry> &eventHandlerRegistry);

  ~AudioEventHandlerRegistryHostObject() override;

  JSI_HOST_FUNCTION_DECL(addAudioEventListener);
  JSI_HOST_FUNCTION_DECL(removeAudioEventListener);

  [[nodiscard]] const std::shared_ptr<IAudioEventHandlerRegistry> &getEventHandlerRegistry() const {
    return eventHandlerRegistry_;
  }

 private:
  std::shared_ptr<IAudioEventHandlerRegistry> eventHandlerRegistry_;
};
} // namespace audioapi
