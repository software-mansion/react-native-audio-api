#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/GraphObject.h>
#include <audioapi/core/utils/graph/InputPool.h>
#include <audioapi/core/utils/graph/NodeHandle.h>

#include <cstdint>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

/// @brief Cache-friendly, index-stable node storage with in-place topological sort.
///
/// Nodes are stored in a flat vector that is kept topologically sorted
/// (sources first, sinks last). The graph supports O(V+E) compaction of
/// orphaned nodes and O(1)-extra-space Kahn's toposort.
///
/// @note Can store at most 2^30 nodes due to bit-packed indices (~10^9).
class AudioGraph {
  // ── Node ────────────────────────────────────────────────────────────────

  struct Node {
    Node() = default;
    explicit Node(std::shared_ptr<NodeHandle> handle) : handle(std::move(handle)) {}

    std::shared_ptr<NodeHandle> handle = nullptr; // owned handle bridging to HostGraph
    std::uint32_t input_head = InputPool::kNull;  // head of input linked list in pool_

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

  // ── Node buffer pre-allocation (main-thread → audio-thread handoff) ─────

  /// @brief Opaque pre-allocated node storage.
  ///
  /// Created on the main thread via makeNodeBuffer(), then handed to the
  /// audio thread via adoptNodeBuffer(). The returned (old) buffer must be
  /// disposed off the audio thread.
  struct NodeBuffer {
    std::vector<Node> data;
    explicit NodeBuffer(std::uint32_t capacity) {
      data.reserve(capacity);
    }
    NodeBuffer() = default;
  };

  /// @brief Allocates a node buffer with the given capacity on the calling thread.
  [[nodiscard]] static NodeBuffer makeNodeBuffer(std::uint32_t capacity) {
    return NodeBuffer(capacity);
  }

  /// @brief Installs a pre-allocated node buffer on the audio thread.
  ///
  /// Moves all live nodes into the pre-allocated buffer (allocation-free:
  /// capacity was ensured on the main thread), swaps it in, and returns
  /// the old buffer for disposal off the audio thread.
  ///
  /// @note Must be called only from the audio thread.
  [[nodiscard]] NodeBuffer adoptNodeBuffer(NodeBuffer preAllocated);

  /// @brief Entry returned by iter() — a reference to the graph object and a view of its inputs.
  template <typename InputsView>
  struct Entry {
    GraphObject &graphObject;
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
  /// Each entry contains a reference to the GraphObject and an immutable view
  /// of its inputs (as references to GraphObject).
  ///
  /// ## Example usage:
  /// ```cpp
  /// for (auto [graphObject, inputs] : graph.iter()) {
  ///   // process graphObject and its inputs
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
  void addNode(std::shared_ptr<NodeHandle> handle);

  /// @brief Recomputes topological order (if dirty), then compacts the graph
  /// by removing orphaned, input-free, destructible nodes.
  ///
  /// When a node is compacted out its `shared_ptr<NodeHandle>` is released
  /// (refcount drops 2 → 1). HostGraph detects this via `use_count() == 1`
  /// and destroys the ghost + GraphObject on the main thread.
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

inline auto AudioGraph::iter() {
  return nodes |
      std::views::filter([](const Node &n) { return n.handle->audioNode->isProcessable(); }) |
      std::views::transform([this](Node &node) {
           return Entry{
               .graphObject = *node.handle->audioNode,
               .inputs = pool_.view(node.input_head) |
                   std::views::transform([this](std::uint32_t idx) -> const GraphObject & {
                           return *nodes[idx].handle->audioNode;
                         }),
           };
         });
}

} // namespace audioapi::utils::graph
