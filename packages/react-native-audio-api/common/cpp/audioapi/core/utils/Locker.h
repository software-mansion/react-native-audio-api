#pragma once

#include <mutex>

namespace audioapi {

// Small easy interface to manage locking
class Locker {
 public:
  Locker() : lockPtr_(nullptr) {}
  explicit Locker(std::mutex &lockPtr) : lockPtr_(&lockPtr) {
    lock();
  }

  ~Locker() {
    unlock();
  }

  explicit operator bool() const {
    return lockPtr_ != nullptr;
  }

  void lock() {
    if (lockPtr_) {
      lockPtr_->lock();
    }
  }

  void unlock() {
    if (lockPtr_) {
      lockPtr_->unlock();
    }
  }

  static Locker tryLock(std::mutex &lock) {
    Locker result = Locker();

    if (lock.try_lock()) {
      result.lockPtr_ = &lock;
    }

    return result;
  }

 private:
  std::mutex *lockPtr_;
};

} // namespace audioapi
