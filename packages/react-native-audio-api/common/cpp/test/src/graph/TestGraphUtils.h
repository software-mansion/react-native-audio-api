#pragma once

#if !RN_AUDIO_API_TEST
#error "RN_AUDIO_API_TEST must be enabled to use TestGraphUtils"
#define RN_AUDIO_API_TEST true // for intellisense
#endif

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/OfflineAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/BridgeNode.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/core/utils/graph/HostNode.h>
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
    return ctx;
  }();
  return context;
}

// ── MockAudioParam ────────────────────────────────────────────────────────
// Real AudioParam for use in graph tests that need valid param pointers.

inline std::shared_ptr<AudioParam> createMockAudioParam() {
  return std::make_shared<AudioParam>(0.0f, -1.0f, 1.0f, getGraphTestContext());
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

  void setProcessable() {
    setProcessableState(PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE);
  }

 private:
  void processNode(int) override {}

  std::atomic<bool> destructible_;
};

// ── NonProcessableMockNode ────────────────────────────────────────────────
// Pure GraphObject subclass that is not processable. Used to test
// iter() filtering at the AudioGraph level without depending on BridgeNode.

struct NonProcessableMockNode : GraphObject {
  [[nodiscard]] bool isProcessable() const override {
    return false;
  }

  [[nodiscard]] bool canBeDestructed() const override {
    return true;
  }
};

// ── MockHostNode ──────────────────────────────────────────────────────────
// RAII wrapper around HostNode for testing the HostNode lifecycle.

class MockHostNode : public HostNode {
 public:
  explicit MockHostNode(std::shared_ptr<Graph> graph, bool destructible = true)
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
      : MockNode(destructible), value(initialValue), processFn_(std::move(processFn)) {
    setProcessable();
  }

  /// @brief Called by the audio thread with an input range from `Graph::iter()`.
  ///
  /// Supports both strongly-typed test ranges and GraphObject-based ranges,
  /// collecting values into a stack buffer with no heap allocation.
  template <std::ranges::input_range R>
  void process(R &&inputs) {
    if (!processFn_)
      return;
    int buf[kMaxInputs];
    size_t n = 0;
    for (const auto &input : inputs) {
      if (n >= kMaxInputs) {
        continue;
      }

      if constexpr (requires { input.value.load(std::memory_order_acquire); }) {
        buf[n++] = input.value.load(std::memory_order_acquire);
      } else {
        auto *typed = dynamic_cast<const ProcessableMockNode *>(&input);
        if (typed) {
          buf[n++] = typed->value.load(std::memory_order_acquire);
        }
      }
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
  static std::pair<AudioGraph, HostGraph> createTestGraph(
      std::vector<std::vector<size_t>> adjacencyList);

  /// @brief Converts AudioGraph to adjacency list for equality comparison.
  static std::vector<std::vector<size_t>> convertAudioGraphToAdjacencyList(
      const AudioGraph &audioGraph);

  /// @brief Converts HostGraph to adjacency list for equality comparison.
  static std::vector<std::vector<size_t>> convertHostGraphToAdjacencyList(
      const HostGraph &hostGraph);

 private:
  static HostGraph makeFromAdjacencyList(const std::vector<std::vector<size_t>> &adjacencyList);

  static AudioGraph createAudioGraphFromHostGraph(const HostGraph &hostGraph);
};

} // namespace audioapi::utils::graph
