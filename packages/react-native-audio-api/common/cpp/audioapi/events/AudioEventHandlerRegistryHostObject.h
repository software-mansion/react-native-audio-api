#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioEventHandlerRegistryHostObject : public JsiHostObject {
 public:
    explicit AudioEventHandlerRegistryHostObject(const std::shared_ptr<AudioEventHandlerRegistry>& eventHandlerRegistry) {
        eventHandlerRegistry_ = eventHandlerRegistry;

        addFunctions(
          JSI_EXPORT_FUNCTION(AudioEventHandlerRegistryHostObject, addAudioEventListener),
          JSI_EXPORT_FUNCTION(AudioEventHandlerRegistryHostObject, removeAudioEventListener));
    }

    JSI_HOST_FUNCTION(addAudioEventListener) {
      auto eventName = args[0].getString(runtime).utf8(runtime);
      auto callback = std::make_shared<jsi::Function>(args[1].getObject(runtime).getFunction(runtime));

      auto listenerId = eventHandlerRegistry_->registerHandler(eventName, callback);

      return jsi::String::createFromUtf8(runtime, std::to_string(listenerId));
    }

    JSI_HOST_FUNCTION(removeAudioEventListener) {
        auto eventName = args[0].getString(runtime).utf8(runtime);
        uint64_t listenerId = std::stoull(args[1].getString(runtime).utf8(runtime));

        eventHandlerRegistry_->unregisterHandler(eventName, listenerId);

        return jsi::Value::undefined();
    }

 private:
    std::shared_ptr<AudioEventHandlerRegistry> eventHandlerRegistry_;
};
} // namespace audioapi

