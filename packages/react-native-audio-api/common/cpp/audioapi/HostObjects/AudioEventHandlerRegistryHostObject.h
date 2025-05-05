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
          JSI_EXPORT_FUNCTION(AudioEventHandlerRegistryHostObject, addAudioEventListener));
    }

    JSI_HOST_FUNCTION(addAudioEventListener) {
      auto eventName = args[0].getString(runtime).utf8(runtime);
      auto callback = std::make_shared<jsi::Function>(args[1].getObject(runtime).getFunction(runtime));

      eventHandlerRegistry_->registerHandler(eventName, 1, callback);

      std::string jsonString = R"({"name":"John Doe","age":30,"isEmployed":true})";
      std::vector<uint8_t> jsonData(jsonString.begin(), jsonString.end());
        auto result = jsi::Value::createFromJsonUtf8(runtime, jsonData.data(), jsonData.size());

      return result;
    }

 private:
    std::shared_ptr<AudioEventHandlerRegistry> eventHandlerRegistry_;
};
} // namespace audioapi

