#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/ITaskExecutor.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/ThreadPool.hpp>

#include <cstddef>
#include <functional>
#include <utility>

namespace audioapi {

/// @brief Several workers behind a round-robin load balancer. Tasks may run concurrently and
/// out of scheduling order, which suits independent jobs such as decoding separate files.
class PooledTaskExecutor : public ITaskExecutor {
 public:
  explicit PooledTaskExecutor(
      size_t workerCount = PROMISE_VENDOR_THREAD_POOL_WORKER_COUNT,
      size_t loadBalancerQueueSize = PROMISE_VENDOR_THREAD_POOL_LOAD_BALANCER_QUEUE_SIZE,
      size_t workerQueueSize = PROMISE_VENDOR_THREAD_POOL_WORKER_QUEUE_SIZE)
      : pool_(workerCount, loadBalancerQueueSize, workerQueueSize) {}
  DELETE_COPY_AND_MOVE(PooledTaskExecutor);
  ~PooledTaskExecutor() override = default;

  void schedule(std::function<void()> &&task) override {
    pool_.schedule(std::move(task));
  }

 private:
  PromiseVendorThreadPool pool_;
};

} // namespace audioapi
