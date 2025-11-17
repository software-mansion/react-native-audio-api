#include <audioapi/jsi/JsiPromise.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace audioapi {

using namespace facebook;

jsi::Value PromiseVendor::createAsyncPromise(std::function<PromiseResolver()> &&function) {
  auto &runtime = *runtime_;
  auto callInvoker = callInvoker_;
  auto threadPool = threadPool_;
  auto promiseCtor = runtime.global().getPropertyAsFunction(runtime, "Promise");
  auto promiseLambda = [threadPool = std::move(threadPool),
                        callInvoker = std::move(callInvoker),
                        function = std::move(function)](
                           jsi::Runtime &runtime,
                           const jsi::Value &thisValue,
                           const jsi::Value *arguments,
                           size_t count) mutable -> jsi::Value {
    auto resolveLocal = arguments[0].asObject(runtime).asFunction(runtime);
    auto resolve = std::make_shared<jsi::Function>(std::move(resolveLocal));
    auto rejectLocal = arguments[1].asObject(runtime).asFunction(runtime);
    auto reject = std::make_shared<jsi::Function>(std::move(rejectLocal));

    threadPool->schedule(
        &PromiseVendor::asyncPromiseJob,
        std::move(callInvoker),
        std::move(function),
        std::move(resolve),
        std::move(reject));
    return jsi::Value::undefined();
  };
  auto promiseFunction = jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, "asyncPromise"), 2, std::move(promiseLambda));
  return promiseCtor.callAsConstructor(runtime, std::move(promiseFunction));
}

jsi::Value PromiseVendor::createAsyncPromise(std::function<void(Promise &&)> &&function) {
  auto &runtime = *runtime_;
  auto callInvoker = callInvoker_;
  auto threadPool = threadPool_;
  auto promiseCtor = runtime.global().getPropertyAsFunction(runtime, "Promise");
  auto promiseLambda = [threadPool = std::move(threadPool),
                        callInvoker = std::move(callInvoker),
                        function = std::move(function)](
                           jsi::Runtime &runtime,
                           const jsi::Value &thisValue,
                           const jsi::Value *arguments,
                           size_t count) mutable -> jsi::Value {
    auto resolveLocal = arguments[0].asObject(runtime).asFunction(runtime);
    auto rejectLocal = arguments[1].asObject(runtime).asFunction(runtime);

    Promise promise(std::move(callInvoker), std::move(resolveLocal), std::move(rejectLocal));

    threadPool->schedule(std::move(function), std::move(promise));

    return jsi::Value::undefined();
  };
  auto promiseFunction = jsi::Function::createFromHostFunction(
      runtime, jsi::PropNameID::forUtf8(runtime, "asyncPromise"), 2, std::move(promiseLambda));
  return promiseCtor.callAsConstructor(runtime, std::move(promiseFunction));
}

void PromiseVendor::asyncPromiseJob(
    std::shared_ptr<react::CallInvoker> callInvoker,
    std::function<PromiseResolver()> &&function,
    std::shared_ptr<jsi::Function> &&resolve,
    std::shared_ptr<jsi::Function> &&reject) {
  auto resolver = function();
  callInvoker->invokeAsync(
      [resolver = std::move(resolver), reject = std::move(reject), resolve = std::move(resolve)](
          jsi::Runtime &runtime) -> void {
        auto result = resolver(runtime);
        if (std::holds_alternative<jsi::Value>(result)) {
          auto valueShared = std::make_shared<jsi::Value>(std::move(std::get<jsi::Value>(result)));
          resolve->call(runtime, *valueShared);
        } else {
          auto errorMessage = std::get<std::string>(result);
          auto error = jsi::JSError(runtime, errorMessage);
          reject->call(runtime, error.value());
        }
      });
}

} // namespace audioapi
