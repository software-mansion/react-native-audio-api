#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

// ContextPromise<> is a special template for no value promises.
// ContextPromise<T> is a general template for value promises.
template <typename... Args>
  requires(sizeof...(Args) <= 1)
class ContextPromiseResolver {
 public:
  ContextPromiseResolver(
      std::function<void(const Args &...)> resolve,
      std::function<void(const std::string &)> reject)
      : resolve_(std::move(resolve)), reject_(std::move(reject)) {}

  void resolve(const Args &...values) const {
    resolve_(values...);
  }

  static void resolve(
      const std::shared_ptr<ContextPromiseResolver<Args...>> &promise,
      const Args &...values) {
    if (promise != nullptr) {
      promise->resolve(values...);
    }
  }

  void reject(const std::string &message) const {
    reject_(message);
  }

  static void reject(
      const std::shared_ptr<ContextPromiseResolver<Args...>> &promise,
      const std::string &message) {
    if (promise != nullptr) {
      promise->reject(message);
    }
  }

 private:
  std::function<void(const Args &...)> resolve_;
  std::function<void(const std::string &)> reject_;
};

} // namespace audioapi
