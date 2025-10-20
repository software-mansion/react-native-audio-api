#include <audioapi/jsi/JsiPromise.h>
#include <stdexcept>

namespace audioapi {

using namespace facebook;

jsi::Value PromiseVendor::createPromise(
    const std::function<void(std::shared_ptr<Promise>)> &function) {
  if (runtime_ == nullptr) {
    throw std::runtime_error("Runtime was null!");
  }

  auto &runtime = *runtime_;
  auto callInvoker = callInvoker_;

  // get Promise constructor
  auto promiseCtor = runtime.global().getPropertyAsFunction(runtime, "Promise");

  // create a "run" function (first Promise arg)
  auto runPromise = jsi::Function::createFromHostFunction(
      runtime,
      jsi::PropNameID::forUtf8(runtime, "runPromise"),
      2,
      [callInvoker, function](
          jsi::Runtime &runtime,
          const jsi::Value &thisValue,
          const jsi::Value *arguments,
          size_t count) -> jsi::Value {
        auto resolveLocal = arguments[0].asObject(runtime).asFunction(runtime);
        auto resolve = std::make_shared<jsi::Function>(std::move(resolveLocal));
        auto rejectLocal = arguments[1].asObject(runtime).asFunction(runtime);
        auto reject = std::make_shared<jsi::Function>(std::move(rejectLocal));

        auto resolveWrapper =
            [resolve, &runtime, callInvoker](
                const std::function<jsi::Value(jsi::Runtime &)> &resolver)
            -> void {
          callInvoker->invokeAsync([resolve, &runtime, resolver]() -> void {
            auto valueShared = std::make_shared<jsi::Value>(resolver(runtime));

            resolve->call(runtime, *valueShared);
          });
        };

        auto rejectWrapper = [reject, &runtime, callInvoker](
                                 const std::string &errorMessage) -> void {
          callInvoker->invokeAsync([reject, &runtime, errorMessage]() -> void {
            auto error = jsi::JSError(runtime, errorMessage);
            reject->call(runtime, error.value());
          });
        };

        auto promise = std::make_shared<Promise>(resolveWrapper, rejectWrapper);
        function(promise);

        return jsi::Value::undefined();
      });

  // return new Promise((resolve, reject) => ...)
  return promiseCtor.callAsConstructor(runtime, runPromise);
}

jsi::Value PromiseVendor::createAsyncPromise(
    std::function<PromiseResolver()> &&function) {
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
      runtime,
      jsi::PropNameID::forUtf8(runtime, "asyncPromise"),
      2,
      std::move(promiseLambda));
  return promiseCtor.callAsConstructor(runtime, std::move(promiseFunction));
}

void PromiseVendor::asyncPromiseJob(
    std::shared_ptr<react::CallInvoker> callInvoker,
    std::function<PromiseResolver()> &&function,
    std::shared_ptr<jsi::Function> &&resolve,
    std::shared_ptr<jsi::Function> &&reject) {
  auto resolver = function();
  callInvoker->invokeAsync(
      [resolver = std::move(resolver),
       reject = std::move(reject),
       resolve = std::move(resolve)](jsi::Runtime &runtime) -> void {
        auto result = resolver(runtime);
        if (std::holds_alternative<jsi::Value>(result)) {
          auto valueShared = std::make_shared<jsi::Value>(
              std::move(std::get<jsi::Value>(result)));
          resolve->call(runtime, *valueShared);
        } else {
          auto errorMessage = std::get<std::string>(result);
          auto error = jsi::JSError(runtime, errorMessage);
          reject->call(runtime, error.value());
        }
      });
}

} // namespace audioapi
