#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/graph/InputPool.hpp>
#include <audioapi/core/utils/graph/NodeHandle.hpp>

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstdint>
#include <iterator>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

template <typename T>
concept AudioGraphNode = std::derived_from<T, ::audioapi::AudioNode>;

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
    std::uint32_t input_head = InputPool::kNull;            // head of input linked list in pool_

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

  AudioGraph(const AudioGraph &) = delete;
  AudioGraph &operator=(const AudioGraph &) = delete;

  AudioGraph(AudioGraph &&) noexcept = default;
  AudioGraph &operator=(AudioGraph &&) noexcept = default;

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

  /// @brief Returns a reference to the input pool used for edge storage.
  [[nodiscard]] InputPool &pool();
  [[nodiscard]] const InputPool &pool() const;

  /// @brief Pre-reserves the internal node vector to at least `capacity`.
  ///
  /// Call from `processGrowEvents()` (outside the allocation-free zone)
  /// so that subsequent `addNode` calls within `processEvents()` do not
  /// trigger vector reallocation.
  void reserveNodes(std::uint32_t capacity);

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
  /// Uses a two-pass approach: pass 1 marks deletions (cascading in topo
  /// order) and computes index remapping; pass 2 remaps inputs and shifts
  /// kept nodes left.
  ///
  /// Time: O(V + E)
  ///
  /// Extra space: O(1) — everything in place.
  void process();

 private:
  std::vector<Node> nodes;       // always kept topologically sorted
  InputPool pool_;               // pool backing all input linked lists
  bool topo_order_dirty = false; // set by markDirty(), cleared by process()

  /// @brief In-place Kahn's toposort (sources first, sinks last).
  ///
  /// Uses `after_compaction_ind` as an embedded FIFO linked-list for the
  /// BFS queue, and cycle-sort for the final permutation.
  ///
  /// Time: O(V + E)
  ///
  /// Extra space: O(1).
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
               pool_.view(node.input_head) |
                   std::views::transform([this](std::uint32_t idx) -> const NodeType & {
                     return *nodes[idx].handle->audioNode;
                   })};
         });
}

template <AudioGraphNode NodeType>
InputPool &AudioGraph<NodeType>::pool() {
  return pool_;
}

template <AudioGraphNode NodeType>
const InputPool &AudioGraph<NodeType>::pool() const {
  return pool_;
}

template <AudioGraphNode NodeType>
void AudioGraph<NodeType>::reserveNodes(std::uint32_t capacity) {
  nodes.reserve(capacity);
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
    topo_order_dirty = false;
    kahn_toposort();
    if (topo_order_dirty) {
      return;
    }
  }

  const auto n = static_cast<std::uint32_t>(nodes.size());

  // ── Pass 1: mark deletions (cascading, left-to-right in topo order) ────
  // A node is deleted when: orphaned && no live inputs && canBeDestructed().
  // Because the array is topologically sorted, removing a source first lets
  // its dependents see the updated input set and potentially cascade.
  for (auto &node : nodes) {
    pool_.removeIf(
        node.input_head, [this](std::uint32_t inp) { return nodes[inp].will_be_deleted; });

    if (node.orphaned && InputPool::isEmpty(node.input_head) &&
        node.handle->audioNode->canBeDestructed()) {
      node.will_be_deleted = true;
    }
  }

  // ── Compute new-position remap (stored in after_compaction_ind) ─────────
  std::uint32_t new_pos = 0;
  for (std::uint32_t i = 0; i < n; i++) {
    if (!nodes[i].will_be_deleted) {
      nodes[i].after_compaction_ind = static_cast<std::int32_t>(new_pos);
      new_pos++;
    }
    // deleted nodes keep after_compaction_ind == -1 (default)
  }

  // ── Pass 2a: remap inputs to post-compaction indices ─────────────────────
  // Must happen BEFORE shifting nodes, because shifting invalidates source
  // positions that later nodes' inputs may still reference.
  for (std::uint32_t e = 0; e < n; e++) {
    if (nodes[e].will_be_deleted)
      continue;
    for (auto &inp : pool_.mutableView(nodes[e].input_head)) {
      inp = static_cast<std::uint32_t>(nodes[inp].after_compaction_ind);
    }
  }

  // ── Pass 2b: compact — shift kept nodes left ───────────────────────────
  std::uint32_t b = 0;
  for (std::uint32_t e = 0; e < n; e++) {
    if (nodes[e].will_be_deleted)
      continue;
    if (b != e) {
      nodes[b] = std::move(nodes[e]);
      nodes[e].input_head = InputPool::kNull; // prevent double-free in truncation
    }
    nodes[b].handle->index = b;
    b++;
  }

  // Truncate — dropping shared_ptr decrements refcount (2 → 1);
  // HostGraph detects this and destroys the ghost on the main thread.
  for (std::uint32_t i = b; i < n; i++) {
    // Free any lingering pool slots (should already be empty for deleted nodes)
    pool_.freeAll(nodes[i].input_head);
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
    for (std::uint32_t inp : pool_.view(nd.input_head))
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

    for (std::uint32_t inp : pool_.view(nodes[idx].input_head)) {
      if (--nodes[inp].topo_out_degree == 0)
        enq(inp);
    }
  }

  // Phase 3: remap input indices to new positions (before nodes move)
  for (auto &nd : nodes) {
    for (std::uint32_t &inp : pool_.mutableView(nd.input_head))
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
