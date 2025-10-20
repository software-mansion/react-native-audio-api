#pragma once
#include <thread>
#include <vector>
#include <functional>
#include <variant>

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
  struct TaskEvent { audioapi::move_only_function<void()> task; };
  using Event = std::variant<TaskEvent, StopEvent>;

  using Sender = channels::spsc::Sender<Event, channels::spsc::OverflowStrategy::WAIT_ON_FULL, channels::spsc::WaitStrategy::ATOMIC_WAIT>;
  using Receiver = channels::spsc::Receiver<Event, channels::spsc::OverflowStrategy::WAIT_ON_FULL, channels::spsc::WaitStrategy::ATOMIC_WAIT>;
public:
  /// @brief Construct a new ThreadPool
  /// @param numThreads The number of worker threads to create
  /// @param loadBalancerQueueSize The size of the load balancer's queue
  /// @param workerQueueSize The size of each worker thread's queue
  ThreadPool(size_t numThreads, size_t loadBalancerQueueSize = 32, size_t workerQueueSize = 32) {
    auto [sender, receiver] = channels::spsc::channel<Event, channels::spsc::OverflowStrategy::WAIT_ON_FULL, channels::spsc::WaitStrategy::ATOMIC_WAIT>(loadBalancerQueueSize);
    loadBalancerSender = std::move(sender);
    std::vector<Sender> workerSenders;
    workerSenders.reserve(numThreads);
    for (size_t i = 0; i < numThreads; ++i) {
      auto [workerSender, workerReceiver] = channels::spsc::channel<Event, channels::spsc::OverflowStrategy::WAIT_ON_FULL, channels::spsc::WaitStrategy::ATOMIC_WAIT>(workerQueueSize);
      workers.emplace_back(&ThreadPool::workerThreadFunc, this, std::move(workerReceiver));
      workerSenders.emplace_back(std::move(workerSender));
    }
    loadBalancerThread = std::thread(&ThreadPool::loadBalancerThreadFunc, this, std::move(receiver), std::move(workerSenders));
  }
  ~ThreadPool() {
    loadBalancerSender.send(StopEvent{});
    loadBalancerThread.join();
    for (auto& worker : workers) {
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
  template<typename Func, typename ... Args, typename = std::enable_if_t<std::is_invocable_r_v<void, Func, Args...>>>
  void schedule(Func &&task, Args &&... args) noexcept {
    auto boundTask = [f = std::forward<Func>(task), ...capturedArgs = std::forward<Args>(args)]() mutable {
      f(std::forward<Args>(capturedArgs)...);
    };
    loadBalancerSender.send(TaskEvent{audioapi::move_only_function<void()>(std::move(boundTask))});
  }

private:
  std::thread loadBalancerThread;
  std::vector<std::thread> workers;
  Sender loadBalancerSender;

  void workerThreadFunc(Receiver &&receiver) {
    Receiver localReceiver = std::move(receiver);
    while (true) {
      auto event = localReceiver.receive();
      /// We use [[unlikely]] and [[likely]] attributes to help the compiler optimize the branching.
      /// we expect most of the time to receive TaskEvent, and rarely StopEvent.
      /// and whenever we receive StopEvent we can burn some cycles as it will not be expected to execute fast.
      if (std::holds_alternative<StopEvent>(event)) [[ unlikely ]] {
        break;
      } else if (std::holds_alternative<TaskEvent>(event)) [[ likely ]] {
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
      if (std::holds_alternative<StopEvent>(event)) [[ unlikely ]] {
        // Propagate stop event to all workers
        for (size_t i = 0; i < localWorkerSenders.size(); ++i) {
          localWorkerSenders[i].send(StopEvent{});
        }
        break;
      } else if (std::holds_alternative<TaskEvent>(event)) [[ likely ]] {
        // Dispatch task to the next worker in round-robin fashion
        auto& taskEvent = std::get<TaskEvent>(event);
        localWorkerSenders[nextWorker].send(std::move(taskEvent));
        nextWorker = (nextWorker + 1) % localWorkerSenders.size();
      }
    }
  }
};

}; // namespace audioapi
