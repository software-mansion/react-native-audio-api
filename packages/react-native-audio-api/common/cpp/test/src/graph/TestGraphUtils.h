#pragma once

#if !RN_AUDIO_API_TEST
#error "RN_AUDIO_API_TEST must be enabled to use TestGraphUtils"
#define RN_AUDIO_API_TEST true // for intellisense
#endif

#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/HostGraph.hpp>
#include <audioapi/core/utils/graph/HostNode.hpp>
#include <atomic>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

// ── MockNode ──────────────────────────────────────────────────────────────
// Minimal type satisfying AudioGraphNode concept. No dependency on AudioNode.

struct MockNode {
  explicit MockNode(bool destructible = true) : destructible_(destructible) {}

  [[nodiscard]] bool canBeDestructed() const {
    return destructible_.load(std::memory_order_acquire);
  }

  /// @brief Thread-safe setter for use in tests.
  void setDestructible(bool value) {
    destructible_.store(value, std::memory_order_release);
  }

 private:
  std::atomic<bool> destructible_;
};

// ── MockHostNode ──────────────────────────────────────────────────────────
// RAII wrapper around HostNode<MockNode> for testing the HostNode lifecycle.

class MockHostNode : public HostNode<MockNode> {
 public:
  explicit MockHostNode(std::shared_ptr<Graph<MockNode>> graph, bool destructible = true)
      : HostNode(std::move(graph), std::make_unique<MockNode>(destructible)) {}
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
