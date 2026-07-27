#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>

namespace audioapi {

class ContextPromise {
 public:
  ContextPromise(std::function<void()> resolve, std::function<void(const std::string &)> reject)
      : resolve_(std::move(resolve)), reject_(std::move(reject)) {}

  void resolve() const {
    resolve_();
  }

  static void resolve(const std::shared_ptr<ContextPromise> &promise) {
    if (promise != nullptr) {
      promise->resolve();
    }
  }

  void reject(const std::string &message) const {
    reject_(message);
  }

  static void reject(const std::shared_ptr<ContextPromise> &promise, const std::string &message) {
    if (promise != nullptr) {
      promise->reject(message);
    }
  }

 private:
  std::function<void()> resolve_;
  std::function<void(const std::string &)> reject_;
};

template <typename T>
class ContextValuePromise {
 public:
  ContextValuePromise(
      std::function<void(const T &)> resolve,
      std::function<void(const std::string &)> reject)
      : resolve_(std::move(resolve)), reject_(std::move(reject)) {}

  void resolve(const T &value) const {
    resolve_(value);
  }

  static void resolve(const std::shared_ptr<ContextValuePromise<T>> &promise, const T &value) {
    if (promise != nullptr) {
      promise->resolve(value);
    }
  }

  void reject(const std::string &message) const {
    reject_(message);
  }

  static void reject(
      const std::shared_ptr<ContextValuePromise<T>> &promise,
      const std::string &message) {
    if (promise != nullptr) {
      promise->reject(message);
    }
  }

 private:
  std::function<void(const T &)> resolve_;
  std::function<void(const std::string &)> reject_;
};

} // namespace audioapi
