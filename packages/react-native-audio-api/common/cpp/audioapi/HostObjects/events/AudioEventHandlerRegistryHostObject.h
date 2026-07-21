#pragma once

#include <audioapi/jsi/HostObject.h>

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioEventHandlerRegistry;

class AudioEventHandlerRegistryHostObject : public HostObject {
 public:
  explicit AudioEventHandlerRegistryHostObject(
      const std::shared_ptr<AudioEventHandlerRegistry> &eventHandlerRegistry);

  JSI_HOST_FUNCTION_DECL(addAudioEventListener);
  JSI_HOST_FUNCTION_DECL(removeAudioEventListener);

  [[nodiscard]] const std::shared_ptr<AudioEventHandlerRegistry> &getEventHandlerRegistry() const {
    return eventHandlerRegistry_;
  }

 private:
  std::shared_ptr<AudioEventHandlerRegistry> eventHandlerRegistry_;
};
} // namespace audioapi
