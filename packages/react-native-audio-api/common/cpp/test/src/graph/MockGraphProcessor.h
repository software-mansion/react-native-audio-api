#pragma once

#include <audioapi/core/utils/graph/Graph.hpp>
#include "AudioThreadGuard.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <thread>

#ifdef __APPLE__
#include <pthread.h>
#elif defined(__linux__)
#include <pthread.h>
#endif

namespace audioapi::test {

/// @brief Simulates a real-time audio thread that continuously processes a Graph.
///
/// Spawns a dedicated thread that repeatedly:
///   1. Processes SPSC events (graph mutations from the main thread)
///   2. Runs toposort + compaction
///   3. Iterates nodes in topological order, calling `process()` if the
///      NodeType supports it (SFINAE via `requires`)
///
/// The thread is instrumented with AudioThreadGuard to detect heap
/// allocations / deallocations and sample context-switch counters.
///
/// ## Usage
/// ```cpp
/// Graph<ProcessableMockNode> graph(4096);
/// MockGraphProcessor processor(graph);
/// processor.start();
/// // … mutate graph from main thread …
/// processor.stop();
/// EXPECT_TRUE(processor.allocationClean());
/// ```
///
/// @tparam NodeType the audio graph node type (must satisfy AudioGraphNode)
template <audioapi::utils::graph::AudioGraphNode NodeType>
class MockGraphProcessor {
 public:
  explicit MockGraphProcessor(audioapi::utils::graph::Graph<NodeType> &graph) : graph_(graph) {}

  ~MockGraphProcessor() {
    stop();
  }

  MockGraphProcessor(const MockGraphProcessor &) = delete;
  MockGraphProcessor &operator=(const MockGraphProcessor &) = delete;

  /// @brief Starts the audio processing thread.
  void start() {
    running_.store(true, std::memory_order_release);
    thread_ = std::thread([this] { run(); });
  }

  /// @brief Signals the audio thread to stop and waits for it to finish.
  void stop() {
    running_.store(false, std::memory_order_release);
    if (thread_.joinable())
      thread_.join();
  }

  /// @brief Total allocation/deallocation violations across all cycles.
  [[nodiscard]] size_t allocationViolations() const {
    return allocationViolations_.load(std::memory_order_acquire);
  }

  /// @brief Total involuntary context switches across all cycles.
  /// @note macOS: process-wide (approximate). Linux: per-thread (exact).
  [[nodiscard]] int64_t involuntaryContextSwitches() const {
    return involuntaryCS_.load(std::memory_order_acquire);
  }

  /// @brief Total voluntary context switches across all cycles.
  [[nodiscard]] int64_t voluntaryContextSwitches() const {
    return voluntaryCS_.load(std::memory_order_acquire);
  }

  /// @brief Number of processing cycles completed.
  [[nodiscard]] size_t cyclesCompleted() const {
    return cycles_.load(std::memory_order_acquire);
  }

  /// @brief True if no allocation violations were detected.
  [[nodiscard]] bool allocationClean() const {
    return allocationViolations() == 0;
  }

 private:
  void run() {
    // Name the thread for debugger / profiler visibility
#ifdef __APPLE__
    pthread_setname_np("MockAudioThread");
#elif defined(__linux__)
    pthread_setname_np(pthread_self(), "MockAudioThread");
#endif

    // Warm up TLS slots so the very first guard check doesn't allocate
    AudioThreadGuard::arm();
    AudioThreadGuard::disarm();

    while (running_.load(std::memory_order_acquire)) {
      // Everything below must be allocation-free.
      {
        AudioThreadGuard::Scope guard;
        graph_.processEvents();

        // Toposort + compact orphaned nodes
        graph_.process();

        // Iterate topological order and call process()
        processNodes();

        // Accumulate stats from this cycle
        allocationViolations_.fetch_add(guard.allocationViolations(), std::memory_order_relaxed);
        involuntaryCS_.fetch_add(guard.involuntaryContextSwitches(), std::memory_order_relaxed);
        voluntaryCS_.fetch_add(guard.voluntaryContextSwitches(), std::memory_order_relaxed);
      }
      cycles_.fetch_add(1, std::memory_order_relaxed);
    }

    // One final drain after stop signal
    graph_.processEvents();
    {
      AudioThreadGuard::Scope guard;
      graph_.process();
      processNodes();

      allocationViolations_.fetch_add(guard.allocationViolations(), std::memory_order_relaxed);
      involuntaryCS_.fetch_add(guard.involuntaryContextSwitches(), std::memory_order_relaxed);
      voluntaryCS_.fetch_add(guard.voluntaryContextSwitches(), std::memory_order_relaxed);
    }
    cycles_.fetch_add(1, std::memory_order_relaxed);
  }

  /// @brief Iterates nodes in topological order and calls process(inputs).
  void processNodes() {
    for (auto &&[node, inputs] : graph_.iter()) {
      if constexpr (requires { node.process(inputs); }) {
        node.process(inputs);
      }
    }
  }

  audioapi::utils::graph::Graph<NodeType> &graph_;
  std::thread thread_;
  std::atomic<bool> running_{false};
  std::atomic<size_t> allocationViolations_{0};
  std::atomic<int64_t> involuntaryCS_{0};
  std::atomic<int64_t> voluntaryCS_{0};
  std::atomic<size_t> cycles_{0};
};

} // namespace audioapi::test
