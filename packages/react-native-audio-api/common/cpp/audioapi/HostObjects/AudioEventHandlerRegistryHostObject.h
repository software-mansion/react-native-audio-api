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

class AudioEventHandlerRegistryHostObject : public JsiHostObject {
 public:
    explicit AudioEventHandlerRegistryHostObject() {
        addFunctions(
          JSI_EXPORT_FUNCTION(AudioEventHandlerRegistryHostObject, on));
    }

    JSI_HOST_FUNCTION(on) {
      auto eventName = args[0].getString(runtime).utf8(runtime);
      auto listenerId = args[1].getNumber();

//      registerHandler(eventName, listenerId, args[2]);

      return jsi::Value::undefined();
    }
};
} // namespace audioapi

