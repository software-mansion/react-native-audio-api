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
    explicit AudioEventHandlerRegistryHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker) {
        runtime_ = runtime;
        callInvoker_ = callInvoker;

        eventNames_ = {"onEnded"};

        for (const auto &eventName : eventNames_) {
          eventHandlers_[eventName] = {};
        }

        addFunctions(
          JSI_EXPORT_FUNCTION(AudioEventHandlerRegistryHostObject, on));
    }

    ~AudioEventHandlerRegistryHostObject() override {
      eventHandlers_.clear();
    }

    void registerHandler(const std::string &eventName, uint64_t listenerId, const jsi::Value &value) {
      eventHandlers_[eventName][listenerId] = std::make_shared<jsi::Function>(value.getObject(*runtime_).getFunction(*runtime_));
    }

    void unregisterHandler(const std::string &eventName, uint64_t listenerId) {
      auto it = eventHandlers_.find(eventName);
      if (it != eventHandlers_.end()) {
        it->second.erase(listenerId);
      }
    }

    template<typename... Args>
    void invokeHandler(const std::string &eventName, Args&&... args) {
        auto it = eventHandlers_.find(eventName);
        if (it != eventHandlers_.end()) {
            for (const auto &pair : it->second) {
                auto handler = pair.second;
                if (handler) {
                    callInvoker_->invokeAsync([this, handler, args...]() {
                        handler->call(*runtime_, args...);
                    });
                }
            }
        }
    }

    JSI_HOST_FUNCTION(on) {
      auto eventName = args[0].getString(*runtime_).utf8(*runtime_);
      auto listenerId = args[1].getNumber();

      registerHandler(eventName, listenerId, args[2]);

      return jsi::Value::undefined();
    }

 private:
    std::shared_ptr<react::CallInvoker> callInvoker_;
    jsi::Runtime *runtime_;
    std::vector<std::string> eventNames_;
    std::unordered_map<std::string, std::unordered_map<uint64_t, std::shared_ptr<jsi::Function>>> eventHandlers_;
};
} // namespace audioapi

