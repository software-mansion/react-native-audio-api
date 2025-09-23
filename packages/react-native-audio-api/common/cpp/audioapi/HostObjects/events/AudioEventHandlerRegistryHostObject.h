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

    JSI_HOST_FUNCTION_DECL(addAudioEventListener);
    JSI_HOST_FUNCTION_DECL(removeAudioEventListener);

 private:
    std::shared_ptr<AudioEventHandlerRegistry> eventHandlerRegistry_;
};
} // namespace audioapi

