#pragma once

#include <audioapi/jsi/JsiHostObject.h>

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioEventHandlerRegistry;

class AudioEventHandlerRegistryHostObject : public JsiHostObject {
 public:
    explicit AudioEventHandlerRegistryHostObject(const std::shared_ptr<AudioEventHandlerRegistry>& eventHandlerRegistry);

    JSI_HOST_FUNCTION_DECL(addAudioEventListener);
    JSI_HOST_FUNCTION_DECL(removeAudioEventListener);

 private:
    std::shared_ptr<AudioEventHandlerRegistry> eventHandlerRegistry_;
};
} // namespace audioapi

