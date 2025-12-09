#pragma once
#include <functional>
#include <memory>
#include <thread>
#include <utility>
#include <variant>
#include <vector>

#include <audioapi/utils/MoveOnlyFunction.hpp>
#include <audioapi/utils/SpscChannel.hpp>

namespace audioapi {

/// @brief A simple thread pool implementation using lock-free SPSC channels for task scheduling and execution.
/// @note The thread pool consists of a load balancer thread and multiple worker threads.
/// @note The load balancer receives tasks and distributes them to worker threads in a round-robin fashion.
/// @note Each worker thread has its own SPSC channel to receive tasks from the load balancer.
/// @note The thread pool can be shut down gracefully by sending a stop event to the load balancer, which then propagates the stop event to all worker threads.
/// @note IMPORTANT: ThreadPool is not thread-safe and events should be scheduled from a single thread only.
class ThreadPool {
  struct StopEvent {};
  struct TaskEvent {
    audioapi::move_only_function<void()> task;
  };
  using Event = std::variant<TaskEvent, StopEvent>;

  struct Cntrl {
    std::atomic<bool> waitingForTasks{false};
    std::atomic<size_t> tasksScheduled{0};
  };

  using Sender = channels::spsc::Sender<
      Event,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>;
  using Receiver = channels::spsc::Receiver<
      Event,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>;

 public:
  /// @brief Construct a new ThreadPool
  /// @param numThreads The number of worker threads to create
  /// @param loadBalancerQueueSize The size of the load balancer's queue
  /// @param workerQueueSize The size of each worker thread's queue
  explicit ThreadPool(
      size_t numThreads,
      size_t loadBalancerQueueSize = 32,
      size_t workerQueueSize = 32) {
    auto [sender, receiver] = channels::spsc::channel<
        Event,
        channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        channels::spsc::WaitStrategy::ATOMIC_WAIT>(loadBalancerQueueSize);
    loadBalancerSender = std::move(sender);
    std::vector<Sender> workerSenders;
    workerSenders.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
      auto [workerSender, workerReceiver] = channels::spsc::channel<
          Event,
          channels::spsc::OverflowStrategy::WAIT_ON_FULL,
          channels::spsc::WaitStrategy::ATOMIC_WAIT>(workerQueueSize);
      workers.emplace_back(&ThreadPool::workerThreadFunc, this, std::move(workerReceiver));
      workerSenders.emplace_back(std::move(workerSender));
    }
    loadBalancerThread = std::thread(
        &ThreadPool::loadBalancerThreadFunc, this, std::move(receiver), std::move(workerSenders));
    controlBlock_ = std::make_unique<Cntrl>();
  }
  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;
  ThreadPool(ThreadPool &&other)
      : loadBalancerThread(std::move(other.loadBalancerThread)),
        workers(std::move(other.workers)),
        loadBalancerSender(std::move(other.loadBalancerSender)),
        controlBlock_(std::move(other.controlBlock_)) {}
  ThreadPool &operator=(ThreadPool &&other) {
    if (this != &other) {
      loadBalancerThread = std::move(other.loadBalancerThread);
      workers = std::move(other.workers);
      loadBalancerSender = std::move(other.loadBalancerSender);
      controlBlock_ = std::move(other.controlBlock_);
      other.movedFrom_ = true;
    }
    return *this;
  }

  ~ThreadPool() {
    if (movedFrom_) {
      return;
    }
    loadBalancerSender.send(StopEvent{});
    loadBalancerThread.join();
    for (auto &worker : workers) {
      worker.join();
    }
  }

  /// @brief Schedule a task to be executed by the thread pool
  /// @tparam Func The type of the task function
  /// @tparam Args The types of the task function arguments
  /// @param task The task function to be executed
  /// @param args The arguments to be passed to the task function
  /// @note This function is lock-free and most of the time wait-free, but may block if the load balancer queue is full.
  /// @note Please remember that the task will be executed in a different thread, so make sure to pass any required variables by value or with std::move.
  /// @note The task should not throw exceptions, as they will not be caught.
  /// @note The task should end at some point, otherwise the thread pool will never be able to shut down.
  /// @note IMPORTANT: This function is not thread-safe and should be called from a single thread only.
  template <
      typename Func,
      typename... Args,
      typename = std::enable_if_t<std::is_invocable_r_v<void, Func, Args...>>>
  void schedule(Func &&task, Args &&...args) noexcept {
    controlBlock_->tasksScheduled.fetch_add(1, std::memory_order_release);

    /// We know that lifetime of each worker thus spsc thus lambda is strongly bounded by ThreadPool lifetime
    /// so we can safely capture control block pointer unsafely here
    Cntrl *cntrl = controlBlock_.get();
    auto boundTask = [cntrl,
                      f = std::forward<Func>(task),
                      ... capturedArgs = std::forward<Args>(args)]() mutable {
      f(std::forward<Args>(capturedArgs)...);
      size_t left = cntrl->tasksScheduled.fetch_sub(1, std::memory_order_acq_rel) - 1;
      if (left == 0) {
        cntrl->waitingForTasks.store(false, std::memory_order_release);
        cntrl->waitingForTasks.notify_one();
      }
    };
    loadBalancerSender.send(TaskEvent{audioapi::move_only_function<void()>(std::move(boundTask))});
  }

  /// @brief Waits for all scheduled tasks to complete
  void wait() {
    /// This logic might seem incorrect at first glance
    /// Main principle for this is that there is only one thread scheduling tasks
    /// If he is waiting for the tasks he CANNOT schedule new tasks so we can assume partial
    /// synchronization here.
    /// We first store true so if any task finishes at this moment he will flip it
    /// Then we check if there are any tasks scheduled
    /// If there are none we can return immediately
    /// If there are some we wait until the last task flips the flag to false
    controlBlock_->waitingForTasks.store(true, std::memory_order_release);
    if (controlBlock_->tasksScheduled.load(std::memory_order_acquire) == 0) {
      controlBlock_->waitingForTasks.store(false, std::memory_order_release);
      return;
    }
    controlBlock_->waitingForTasks.wait(true, std::memory_order_acquire);
    return;
  }

 private:
  std::thread loadBalancerThread;
  std::vector<std::thread> workers;
  Sender loadBalancerSender;
  std::unique_ptr<Cntrl> controlBlock_;
  bool movedFrom_ = false;

  void workerThreadFunc(Receiver &&receiver) {
    Receiver localReceiver = std::move(receiver);
    while (true) {
      auto event = localReceiver.receive();
      /// We use [[unlikely]] and [[likely]] attributes to help the compiler optimize the branching.
      /// we expect most of the time to receive TaskEvent, and rarely StopEvent.
      /// and whenever we receive StopEvent we can burn some cycles as it will not be expected to execute fast.
      if (std::holds_alternative<StopEvent>(event)) [[unlikely]] {
        break;
      } else if (std::holds_alternative<TaskEvent>(event)) [[likely]] {
        std::get<TaskEvent>(event).task();
      }
    }
  }

  void loadBalancerThreadFunc(Receiver &&receiver, std::vector<Sender> &&workerSenders) {
    Receiver localReceiver = std::move(receiver);
    std::vector<Sender> localWorkerSenders = std::move(workerSenders);
    size_t nextWorker = 0;
    while (true) {
      auto event = localReceiver.receive();
      /// We use [[unlikely]] and [[likely]] attributes to help the compiler optimize the branching.
      /// we expect most of the time to receive TaskEvent, and rarely StopEvent.
      /// and whenever we receive StopEvent we can burn some cycles as it will not be expected to execute fast.
      if (std::holds_alternative<StopEvent>(event)) [[unlikely]] {
        // Propagate stop event to all workers
        for (size_t i = 0; i < localWorkerSenders.size(); ++i) {
          localWorkerSenders[i].send(StopEvent{});
        }
        break;
      } else if (std::holds_alternative<TaskEvent>(event)) [[likely]] {
        // Dispatch task to the next worker in round-robin fashion
        auto &taskEvent = std::get<TaskEvent>(event);
        localWorkerSenders[nextWorker].send(std::move(taskEvent));
        nextWorker = (nextWorker + 1) % localWorkerSenders.size();
      }
    }
  }
};

}; // namespace audioapi
