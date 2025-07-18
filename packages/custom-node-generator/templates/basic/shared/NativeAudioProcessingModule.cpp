#include "NativeAudioProcessingModule.h"
#include "MyProcessorNodeHostObject.h"
#include <iostream>
#include <functional>
#include <memory>
#include <audioapi/HostObjects/AudioContextHostObject.h>
#include "MyProcessorNode.h"

namespace facebook::react {

NativeAudioProcessingModule::NativeAudioProcessingModule(std::shared_ptr<CallInvoker> jsInvoker)
    : NativeAudioProcessingModuleCxxSpec(std::move(jsInvoker)) {}

void NativeAudioProcessingModule::injectCustomProcessorInstaller(jsi::Runtime &runtime) {
  auto installer = createInstaller(runtime);
  runtime.global().setProperty(runtime, "createCustomProcessorNode", installer);
}

jsi::Function NativeAudioProcessingModule::createInstaller(jsi::Runtime &runtime) {
  return jsi::Function::createFromHostFunction(
      runtime,
      jsi::PropNameID::forAscii(runtime, "createCustomProcessorNode"),
      0,
      [](jsi::Runtime &runtime, const jsi::Value &thisVal, const jsi::Value *args, size_t count) {
        auto object = args[0].getObject(runtime);
        auto context = object.getHostObject<audioapi::AudioContextHostObject>(runtime);
        if (context != nullptr) {
          auto node = std::make_shared<audioapi::MyProcessorNode>(context->context_.get());
          auto nodeHostObject = std::make_shared<audioapi::MyProcessorNodeHostObject>(node);
          return jsi::Object::createFromHostObject(runtime, nodeHostObject);
        }
        return jsi::Object::createFromHostObject(runtime, nullptr);
      });
    }
} // namespace facebook::react
