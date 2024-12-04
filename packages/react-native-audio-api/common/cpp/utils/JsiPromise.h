#pragma once

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <utility>
#include <memory>
#include <string>

namespace JsiPromise {

using namespace facebook;

class Promise {
 public:
  Promise(std::function<void(jsi::Value)> resolve, std::function<void(const std::string&)> reject): _resolve(std::move(resolve)), _reject(std::move(reject)) {}

  bool isResolved;
  void resolve(jsi::Value&& value) {
    _resolve(std::forward<jsi::Value>(value));
  }
  void reject(const std::string& errorMessage) {
    _reject(errorMessage);
  }

 private:
  std::function<void(jsi::Value)> _resolve;
  std::function<void(const std::string&)> _reject;
};

class PromiseVendor {
 public:
  PromiseVendor(jsi::Runtime* runtime, std::shared_ptr<react::CallInvoker> callInvoker): _runtime(runtime), _callInvoker(callInvoker) {}

 public:
  jsi::Value createPromise(std::function<void(std::shared_ptr<Promise>)> func);

 private:
  jsi::Runtime* _runtime;
  std::shared_ptr<react::CallInvoker> _callInvoker;
};

} // namespace JsiPromise
