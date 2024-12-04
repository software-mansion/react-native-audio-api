#include "JsiPromise.h"

#include <jsi/jsi.h>
#include <ReactCommon/CallInvoker.h>
#include <functional>

namespace JsiPromise {

using namespace facebook;

jsi::Value PromiseVendor::createPromise(std::function<void(std::shared_ptr<Promise>)> func) {
  if (_runtime == nullptr) {
    throw new std::runtime_error("Runtime was null!");
  }
  auto& runtime = *_runtime;
  auto callInvoker = _callInvoker;

  // get Promise constructor
  auto promiseCtor = runtime.global().getPropertyAsFunction(runtime, "Promise");

  // create a "run" function (first Promise arg)
  auto runPromise = jsi::Function::createFromHostFunction(runtime,
                                                          jsi::PropNameID::forUtf8(runtime, "runPromise"),
                                                          2,
                                                          [callInvoker, func](jsi::Runtime& runtime,
                                                                              const jsi::Value& thisValue,
                                                                              const jsi::Value* arguments,
                                                                              size_t count) -> jsi::Value {
    auto resolveLocal = arguments[0].asObject(runtime).asFunction(runtime);
    auto resolve = std::make_shared<jsi::Function>(std::move(resolveLocal));
    auto rejectLocal = arguments[1].asObject(runtime).asFunction(runtime);
    auto reject = std::make_shared<jsi::Function>(std::move(rejectLocal));

    auto resolveWrapper = [resolve, &runtime, callInvoker](jsi::Value value) -> void {
      auto valueShared = std::make_shared<jsi::Value>(std::move(value));
      callInvoker->invokeAsync([resolve, &runtime, valueShared]() -> void {
        resolve->call(runtime, *valueShared);
      });
    };

    auto rejectWrapper = [reject, &runtime, callInvoker](const std::string& errorMessage) -> void {
      auto error = jsi::JSError(runtime, errorMessage);
      auto errorShared = std::make_shared<jsi::JSError>(error);
      callInvoker->invokeAsync([reject, &runtime, errorShared]() -> void {
        reject->call(runtime, errorShared->value());
      });
    };

    auto promise = std::make_shared<Promise>(resolveWrapper, rejectWrapper);
    func(promise);

    return jsi::Value::undefined();
  });

  // return new Promise((resolve, reject) => ...)
  return promiseCtor.callAsConstructor(runtime, runPromise);
}

} // namespace JsiPromise
