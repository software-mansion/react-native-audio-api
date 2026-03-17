#pragma once

#if !RN_AUDIO_API_TEST
#error "RN_AUDIO_API_TEST must be enabled to use TestGraphUtils"
#define RN_AUDIO_API_TEST true // for intellisense
#endif

#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/HostGraph.hpp>
#include <audioapi/core/utils/graph/HostNode.hpp>
#include <test/src/MockAudioEventHandlerRegistry.h>
#include <atomic>
#include <functional>
#include <memory>
#include <ranges>
#include <span>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

// ── MockNode ──────────────────────────────────────────────────────────────
// Minimal AudioNode-derived type used by graph tests.

inline std::shared_ptr<BaseAudioContext> getGraphTestContext() {
  static std::shared_ptr<BaseAudioContext> context = [] {
    auto eventRegistry = std::make_shared<MockAudioEventHandlerRegistry>();
    auto ctx =
        std::make_shared<OfflineAudioContext>(2, 1024, 44100.0f, eventRegistry, RuntimeRegistry{});
    ctx->initialize();
    return ctx;
  }();
  return context;
}

struct MockNode : AudioNode {
  explicit MockNode(bool destructible = true)
      : AudioNode(getGraphTestContext()), destructible_(destructible) {}

  [[nodiscard]] bool canBeDestructed() const override {
    return destructible_.load(std::memory_order_acquire);
  }

  /// @brief Thread-safe setter for use in tests.
  void setDestructible(bool value) {
    destructible_.store(value, std::memory_order_release);
  }

 private:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBus,
      int) override {
    return processingBus;
  }

  std::atomic<bool> destructible_;
};

// ── MockHostNode ──────────────────────────────────────────────────────────
// RAII wrapper around HostNode<MockNode> for testing the HostNode lifecycle.

class MockHostNode : public HostNode<MockNode> {
 public:
  explicit MockHostNode(std::shared_ptr<Graph<MockNode>> graph, bool destructible = true)
      : HostNode(std::move(graph), std::make_unique<MockNode>(destructible)) {}
};

// ── ProcessableMockNode ──────────────────────────────────────────────────
// MockNode that carries an integer value and a user-provided callback
// invoked during audio processing.
//
// `process(inputs)` accepts any range of `const ProcessableMockNode&`
// (the inputs view from iter()), reads their values into a small stack
// buffer, and passes them to the callback. Zero heap allocation.

struct ProcessableMockNode : MockNode {
  /// @brief Callback: receives a span of input values, returns the new value.
  using ProcessFn = std::function<int(std::span<const int>)>;

  /// @brief The node's current output value, readable from any thread.
  std::atomic<int> value{0};

  static constexpr size_t kMaxInputs = 128;

  explicit ProcessableMockNode(
      ProcessFn processFn = nullptr,
      int initialValue = 0,
      bool destructible = true)
      : MockNode(destructible), value(initialValue), processFn_(std::move(processFn)) {}

  /// @brief Called by the audio thread with the inputs range from iter().
  /// Collects input values into a stack buffer — no heap allocation.
  template <std::ranges::input_range R>
  void process(R &&inputs) {
    if (!processFn_)
      return;
    int buf[kMaxInputs];
    size_t n = 0;
    for (const auto &input : inputs) {
      if (n < kMaxInputs)
        buf[n++] = input.value.load(std::memory_order_acquire);
    }
    value.store(processFn_({buf, n}), std::memory_order_release);
  }

 private:
  ProcessFn processFn_;
};

// ── TestGraphUtils ────────────────────────────────────────────────────────

class TestGraphUtils {
 public:
  /// @brief Creates a paired AudioGraph + HostGraph from an adjacency list.
  /// @param adjacencyList adjacencyList[i] = {j, k} means edges i→j, i→k
  /// @return (AudioGraph, HostGraph) pair with consistent structure
  static std::pair<AudioGraph<MockNode>, HostGraph<MockNode>> createTestGraph(
      std::vector<std::vector<size_t>> adjacencyList);

  /// @brief Converts AudioGraph to adjacency list for equality comparison.
  static std::vector<std::vector<size_t>> convertAudioGraphToAdjacencyList(
      const AudioGraph<MockNode> &audioGraph);

  /// @brief Converts HostGraph to adjacency list for equality comparison.
  static std::vector<std::vector<size_t>> convertHostGraphToAdjacencyList(
      const HostGraph<MockNode> &hostGraph);

 private:
  static HostGraph<MockNode> makeFromAdjacencyList(
      const std::vector<std::vector<size_t>> &adjacencyList);

  static AudioGraph<MockNode> createAudioGraphFromHostGraph(const HostGraph<MockNode> &hostGraph);
};

} // namespace audioapi::utils::graph
