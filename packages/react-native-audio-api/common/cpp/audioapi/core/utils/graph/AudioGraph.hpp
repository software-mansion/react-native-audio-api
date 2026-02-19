#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/graph/NodeHandle.hpp>

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <iterator>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

template <typename T>
concept AudioGraphNode = requires(T t) {
  { t.canBeDestructed() } -> std::convertible_to<bool>;
};

/// @brief Cache-friendly, index-stable node storage with in-place topological sort.
///
/// Nodes are stored in a flat vector that is kept topologically sorted
/// (sources first, sinks last). The graph supports O(V+E) compaction of
/// orphaned nodes and O(1)-extra-space Kahn's toposort.
///
/// @note Can store at most 2^30 nodes due to bit-packed indices (~10^9).
template <AudioGraphNode NodeType>
class AudioGraph {
  // ── Node ────────────────────────────────────────────────────────────────

  struct Node {
    Node() = default;
    explicit Node(std::shared_ptr<NodeHandle<NodeType>> handle) : handle(handle) {}

    std::shared_ptr<NodeHandle<NodeType>> handle = nullptr; // owned handle bridging to HostGraph
    std::vector<std::uint32_t> inputs;                      // indices of input nodes

    std::uint32_t topo_out_degree : 31 = 0; // scratch — Kahn's out-degree counter
    unsigned will_be_deleted : 1 = 0;       // scratch — marked for compaction removal
    std::int32_t after_compaction_ind : 31 =
        -1; // scratch — new index after compaction / BFS linked-list next

    /// Node is removed when: orphaned && inputs.empty() && canBeDestructed()
    unsigned orphaned : 1 = 0; // means this node was removed from host graph

#if RN_AUDIO_API_TEST
    size_t test_node_identifier__ = 0;
#endif
  };

 public:
  AudioGraph() = default;
  ~AudioGraph() = default;

  /// @brief Entry returned by iter() — a reference to the audio node and a view of its inputs.
  template <typename InputsView>
  struct Entry {
    NodeType &audioNode;
    InputsView inputs;
  };

  // ── Accessors ───────────────────────────────────────────────────────────

  /// @brief Access node by flat-vector index.
  [[nodiscard]] Node &operator[](std::uint32_t index);

  /// @brief Access node by flat-vector index (const).
  [[nodiscard]] const Node &operator[](std::uint32_t index) const;

  /// @brief Number of live nodes in the graph.
  [[nodiscard]] size_t size() const;

  /// @brief Whether the graph is empty.
  [[nodiscard]] bool empty() const;

  /// @brief Provides an iterable view of the nodes in topological order.
  ///
  /// Each entry contains a reference to the AudioNode and an immutable view
  /// of its inputs (as references to AudioNodes).
  ///
  /// ## Example usage:
  /// ```cpp
  /// for (auto [audioNode, inputs] : graph.iter()) {
  ///   // process audioNode and its inputs
  /// }
  /// ```
  /// @note Lifetime of entries is bound to this graph — they are not owned.
  /// @note Using this iterator after modifying the graph is undefined behavior.
  [[nodiscard]] auto iter();

  // ── Mutators ────────────────────────────────────────────────────────────

  /// @brief Marks the topological ordering as dirty so the next process()
  /// recomputes it.
  void markDirty();

  /// @brief Adds a new node. AudioGraph takes shared ownership of the handle.
  /// @param handle shared NodeHandle bridging to HostGraph
  void addNode(std::shared_ptr<NodeHandle<NodeType>> handle);

  /// @brief Recomputes topological order (if dirty), then compacts the graph
  /// by removing orphaned, input-free, destructible nodes.
  ///
  /// When a node is compacted out its `shared_ptr<NodeHandle>` is released
  /// (refcount drops 2 → 1). HostGraph detects this via `use_count() == 1`
  /// and destroys the ghost + AudioNode on the main thread.
  ///
  /// Time: O(V + E) &nbsp; Space: O(1) — everything in place.
  void process();

 private:
  std::vector<Node> nodes;       // always kept topologically sorted
  bool topo_order_dirty = false; // set by markDirty(), cleared by process()

  /// @brief In-place Kahn's toposort (sources first, sinks last).
  ///
  /// Uses `after_compaction_ind` as an embedded FIFO linked-list for the
  /// BFS queue, and cycle-sort for the final permutation.
  ///
  /// Time: O(V + E) &nbsp; Extra space: O(1).
  void kahn_toposort();
};

// =========================================================================
// Implementation
// =========================================================================

// ── Accessors ─────────────────────────────────────────────────────────────

template <AudioGraphNode NodeType>
auto AudioGraph<NodeType>::operator[](std::uint32_t index) -> Node & {
  return nodes[index];
}

template <AudioGraphNode NodeType>
auto AudioGraph<NodeType>::operator[](std::uint32_t index) const -> const Node & {
  return nodes[index];
}

template <AudioGraphNode NodeType>
size_t AudioGraph<NodeType>::size() const {
  return nodes.size();
}

template <AudioGraphNode NodeType>
bool AudioGraph<NodeType>::empty() const {
  return nodes.empty();
}

template <AudioGraphNode NodeType>
auto AudioGraph<NodeType>::iter() {
  return nodes | std::views::transform([this](Node &node) {
           return Entry{
               *node.handle->audioNode,
               node.inputs | std::views::transform([this](std::uint32_t idx) -> const NodeType & {
                 return *nodes[idx].handle->audioNode;
               })};
         });
}

// ── Mutators ──────────────────────────────────────────────────────────────

template <AudioGraphNode NodeType>
void AudioGraph<NodeType>::markDirty() {
  topo_order_dirty = true;
}

template <AudioGraphNode NodeType>
void AudioGraph<NodeType>::addNode(std::shared_ptr<NodeHandle<NodeType>> handle) {
  handle->index = static_cast<std::uint32_t>(nodes.size());
  nodes.emplace_back(std::move(handle));
}

template <AudioGraphNode NodeType>
void AudioGraph<NodeType>::process() {
  if (topo_order_dirty) {
    kahn_toposort();
    topo_order_dirty = false;
  }

  // Mark nodes for deletion and compact
  std::uint32_t b = 0; // begin of moving window
  std::uint32_t e = 0; // end of moving window

  for (auto &node : nodes) {
    // Remove deleted inputs
    node.inputs.erase(
        std::remove_if(
            node.inputs.begin(),
            node.inputs.end(),
            [this](std::uint32_t inp) { return nodes[inp].will_be_deleted; }),
        node.inputs.end());

    // Check if node qualifies for removal
    if (node.orphaned && node.inputs.empty() && node.handle->audioNode->canBeDestructed()) {
      node.will_be_deleted = true;
      e += 1;
      continue;
    }

    // Remap inputs to post-compaction indices
    for (std::uint32_t &inp : node.inputs) {
      if (nodes[inp].after_compaction_ind == -1)
        continue;
      inp = static_cast<std::uint32_t>(nodes[inp].after_compaction_ind);
    }

    if (b != e) {
      std::swap(nodes[b], nodes[e]);
      nodes[b].handle->index = b;
      nodes[e].after_compaction_ind = static_cast<std::int32_t>(b);
    }
    b++;
    e++;
  }

  // Truncate — dropping shared_ptr decrements refcount (2 → 1);
  // HostGraph detects this and destroys the ghost on the main thread.
  for (std::uint32_t i = b; i < e; i++) {
    nodes[i].handle = nullptr;
  }
  nodes.resize(b);

  // Reset scratch fields for next compaction
  for (auto &node : nodes) {
    node.after_compaction_ind = -1;
    node.will_be_deleted = false;
  }
}

// ── Kahn's toposort ───────────────────────────────────────────────────────

template <AudioGraphNode NodeType>
void AudioGraph<NodeType>::kahn_toposort() {
  const auto n = static_cast<std::uint32_t>(nodes.size());
  if (n <= 1)
    return;

  // Phase 1: compute out-degree
  for (const auto &nd : nodes) {
    for (std::uint32_t inp : nd.inputs)
      nodes[inp].topo_out_degree++;
  }

  // Phase 2: reverse Kahn BFS — sinks first, sources last in dequeue order.
  // FIFO queue embedded as a linked list through after_compaction_ind.
  std::int32_t qh = -1, qt = -1;
  auto enq = [&](std::uint32_t i) {
    nodes[i].after_compaction_ind = -1;
    if (qh == -1) [[unlikely]] {
      qh = qt = static_cast<std::int32_t>(i);
    } else {
      nodes[qt].after_compaction_ind = static_cast<std::int32_t>(i);
      qt = static_cast<std::int32_t>(i);
    }
  };

  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].topo_out_degree == 0)
      enq(i);
  }

  std::uint32_t write = n;
  while (qh != -1) {
    auto idx = static_cast<std::uint32_t>(qh);
    qh = nodes[idx].after_compaction_ind;
    nodes[idx].after_compaction_ind = static_cast<std::int32_t>(--write);

    for (std::uint32_t inp : nodes[idx].inputs) {
      if (--nodes[inp].topo_out_degree == 0)
        enq(inp);
    }
  }

  // Phase 3: remap input indices to new positions (before nodes move)
  for (auto &nd : nodes) {
    for (std::uint32_t &inp : nd.inputs)
      inp = static_cast<std::uint32_t>(nodes[inp].after_compaction_ind);
  }

  // Phase 4: apply permutation in place via cycle sort
  for (std::uint32_t i = 0; i < n; i++) {
    while (nodes[i].after_compaction_ind != static_cast<std::int32_t>(i)) {
      auto t = static_cast<std::uint32_t>(nodes[i].after_compaction_ind);
      std::swap(nodes[i], nodes[t]);
    }
  }

  // Phase 5: update handle indices & reset scratch
  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].handle)
      nodes[i].handle->index = i;
    nodes[i].after_compaction_ind = -1;
  }
}

} // namespace audioapi::utils::graph
