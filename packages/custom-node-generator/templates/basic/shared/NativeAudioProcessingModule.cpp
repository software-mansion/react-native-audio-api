#include "NativeAudioProcessingModule.h"
#include "MyProcessorNodeHostObject.h"

#include <audioapi/compatibility/StableAPI.h>

#include <functional>
#include <iostream>
#include <memory>

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
        auto host = object.getHostObject<audioapi::BaseAudioContextHostObject>(runtime);
        if (host != nullptr) {
          return jsi::Object::createFromHostObject(
              runtime, std::make_shared<audioapi::MyProcessorNodeHostObject>(host->getContext()));
        }
        return jsi::Object::createFromHostObject(runtime, nullptr);
      });
}

} // namespace facebook::react
