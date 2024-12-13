#pragma once

#include <ReactCommon/CallInvoker.h>
#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

using namespace facebook;

class Promise {
 public:
  Promise(
      std::function<void(jsi::Value)> resolve,
      std::function<void(const std::string &)> reject)
      : resolve_(std::move(resolve)), reject_(std::move(reject)) {}

  void resolve(jsi::Value &&value) {
    resolve_(std::forward<jsi::Value>(value));
  }
  void reject(const std::string &errorMessage) {
    reject_(errorMessage);
  }

 private:
  std::function<void(jsi::Value)> resolve_;
  std::function<void(const std::string &)> reject_;
};

class PromiseVendor {
 public:
  PromiseVendor(
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker)
      : runtime_(runtime), callInvoker_(callInvoker) {}

 public:
  jsi::Value createPromise(
      const std::function<void(std::shared_ptr<Promise>)> &function);

 private:
  jsi::Runtime *runtime_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
};

} // namespace audioapi
