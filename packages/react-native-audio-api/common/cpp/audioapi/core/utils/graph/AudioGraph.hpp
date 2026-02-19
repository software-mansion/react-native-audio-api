#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/graph/Disposer.hpp>
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

/// @brief Cache-friendly, index-stable node storage
///
/// Nodes are stored in a flat vector. Which is topologically sorted.
///
/// @note it can store at most 2^30 nodes due to bit packaging of indexes (aprox 10^9, it will be more than enough for all practical purposes)
class AudioGraph {
  struct Node {
    Node() = default;
    explicit Node(std::shared_ptr<NodeHandle> handle) : handle(handle) {}

    std::shared_ptr<NodeHandle> handle = nullptr; // owned handle bridging to HostGraph
    std::vector<std::uint32_t> inputs;            // indices of input nodes

    std::uint32_t topo_out_degree : 31 = 0; // scratch space for topological sort
    unsigned will_be_deleted : 1 = 0;       // scratch space for graph traversals
    std::int32_t after_compaction_ind : 31 =
        -1; // used during compaction to store the new index of the node, -1 means the node wasn't moved

    // Node removal rules:
    // - when we loose a handle from host object it is marked as orphaned
    // - if:
    //    - node is orphaned
    //    - has no inputs
    //    - node.canBeDestructed() == true
    // then we can safely remove it from the graph and schedule for destruction in audio graph.
    unsigned orphaned : 1 = 0; // means this node was removed from host graph

#if RN_AUDIO_API_TEST
    size_t test_node_identifier__ = 0;
#endif // RN_AUDIO_API_TEST
  };

 public:
  AudioGraph() = default;
  ~AudioGraph() = default;

  template <typename InputsView>
  struct Entry {
    AudioNode &audioNode;
    InputsView inputs; // immutable view of the node's inputs
  };

  [[nodiscard]] Node &operator[](std::uint32_t index) {
    return nodes[index];
  }

  [[nodiscard]] const Node &operator[](std::uint32_t index) const {
    return nodes[index];
  }

  [[nodiscard]] size_t size() const {
    return nodes.size();
  }

  [[nodiscard]] bool empty() const {
    return nodes.empty();
  }

  /// @brief Provides an iterable view of the nodes in topological order
  /// Each entry contains a reference to the AudioNode and an immutable view of its inputs (as references to AudioNodes).
  /// @return iterable view
  ///
  /// ## Example usage:
  /// ```cpp
  /// for (auto [audioNode, inputs] : nodeStorage.iter()) {
  ///   // process audioNode and its inputs
  /// }
  /// ```
  /// @note IMPORTANT: lifetime of the entries is bound to the lifetime of the graph, these are not owned
  /// @note IMPORTANT: using this iterator after modifying the graph is undefined behavior
  [[nodiscard]] auto iter() {
    return nodes | std::views::transform([this](Node &node) {
             return Entry{
                 *node.handle->audioNode,
                 node.inputs |
                     std::views::transform([this](std::uint32_t idx) -> const AudioNode & {
                       return *nodes[idx].handle->audioNode;
                     })};
           });
  }

  /// @brief Marks the topological ordering dirty
  void markDirty() {
    topo_order_dirty = true;
  }

  /// @brief Adds a new node to the storage
  /// @param handle owned NodeHandle pointer
  /// @note IMPORTANT: AudioGraph takes ownership of the handle
  void addNode(std::shared_ptr<NodeHandle> handle) {
    handle->index = static_cast<std::uint32_t>(nodes.size());
    nodes.emplace_back(std::move(handle));
  }

  /// @brief Preprocesses the graph by recomputing topological order if needed, and performing node deletion and compaction.
  /// @param disposer
  /// Time complexity: O(V + E) - we visit each node once for topological sort and once for compaction, and we visit each edge once during topological sort and once during compaction when we remap inputs
  /// Space complexity: O(1) - at runtime we are doing everything in place, no extra allocations are performed
  template <size_t D>
    requires(D >= sizeof(std::unique_ptr<NodeHandle>) && D >= sizeof(std::vector<std::uint32_t>))
  void process(Disposer<D> &disposer) {
    if (topo_order_dirty) {
      kahn_toposort();
      topo_order_dirty = false;
    }

    // mark nodes for deletion and compact
    std::uint32_t b = 0; // begin of moving window
    std::uint32_t e = 0; // end of moving window

    for (auto &node : nodes) {
      // remove all removed inputs from the node.inputs vector
      node.inputs.erase(
          std::remove_if(
              node.inputs.begin(),
              node.inputs.end(),
              [this](std::uint32_t inp) { return nodes[inp].will_be_deleted; }),
          node.inputs.end());

      // if node is orphaned, has no inputs and can be destructed we can mark it for deletion
      if (node.orphaned && node.inputs.empty() && node.handle->audioNode->canBeDestructed()) {
        node.will_be_deleted = true;
        e += 1;
        continue;
      }

      // remap all inputs of the node to the new indices
      for (std::uint32_t &inp : node.inputs) {
        if (nodes[inp].after_compaction_ind == -1)
          continue; // input wasn't moved, no need to remap
        inp = static_cast<std::uint32_t>(nodes[inp].after_compaction_ind);
      }

      if (b != e) {
        std::swap(nodes[b], nodes[e]);
        nodes[b].handle->index = b; // update handle to reflect new position
        nodes[e].after_compaction_ind = static_cast<std::int32_t>(
            b); // store new index in the node that we just moved to the end of vector, so we can remap it later if needed
      }
      b++;
      e++;
    }

    // now all live nodes are in the beginning of vector and we can just truncate it
    for (std::uint32_t i = b; i < e; i++) {
      disposer.dispose(
          std::move(
              nodes[i]
                  .handle)); // schedule handle for deletion on audio thread, it will also delete the AudioNode owned by the handle
      disposer.dispose(std::move(nodes[i].inputs)); // free vector heap allocation off audio thread
      nodes[i].handle = nullptr;
    }
    nodes.resize(b);
    for (auto &node : nodes) {
      node.after_compaction_ind =
          -1; // reset after_compaction index for all nodes, we will need it for the next compaction
      node.will_be_deleted =
          false; // reset will_be_deleted flag for all nodes, we will need it for the next compaction
    }
  }

 private:
  std::vector<Node> nodes; // always topologically sorted
  bool topo_order_dirty =
      false; // whether any operation has potentially invalidated the execution order

  /// @brief Performs Kahn's algorithm to sort topologically in place (sources first, sinks last)
  /// Time: O(V + E), Extra space: O(1) — the BFS queue is embedded in after_compaction_ind as a linked list,
  /// and the resulting permutation is applied via cycle sort.
  void kahn_toposort() {
    const auto n = static_cast<std::uint32_t>(nodes.size());
    if (n <= 1)
      return;

    // ── Phase 1: compute out-degree (how many other nodes list this one as input) ──
    for (auto &nd : nodes)
      nd.topo_out_degree = 0;
    for (const auto &nd : nodes) {
      for (std::uint32_t inp : nd.inputs)
        nodes[inp].topo_out_degree++;
    }

    // ── Phase 2: reverse Kahn BFS (sinks first → sources last in dequeue order) ──
    // We embed a FIFO queue as a singly-linked list through after_compaction_ind.
    std::int32_t qh = -1, qt = -1; // queue head / tail
    auto enq = [&](std::uint32_t i) {
      nodes[i].after_compaction_ind = -1; // end-of-list sentinel
      if (qh == -1) {
        qh = qt = static_cast<std::int32_t>(i);
      } else {
        nodes[qt].after_compaction_ind = static_cast<std::int32_t>(i);
        qt = static_cast<std::int32_t>(i);
      }
    };

    for (std::uint32_t i = 0; i < n; i++) {
      if (nodes[i].topo_out_degree == 0)
        enq(i); // seed with sinks
    }

    std::uint32_t write = n; // fill from the end → sinks land last
    while (qh != -1) {
      auto idx = static_cast<std::uint32_t>(qh);
      qh = nodes[idx].after_compaction_ind; // pop head
      nodes[idx].after_compaction_ind =
          static_cast<std::int32_t>(--write); // record target position

      for (std::uint32_t inp : nodes[idx].inputs) {
        if (--nodes[inp].topo_out_degree == 0)
          enq(inp);
      }
    }

    // ── Phase 3: remap input indices to new positions (before nodes move) ──
    for (auto &nd : nodes) {
      for (std::uint32_t &inp : nd.inputs)
        inp = static_cast<std::uint32_t>(nodes[inp].after_compaction_ind);
    }

    // ── Phase 4: apply permutation in place via cycle sort ──
    for (std::uint32_t i = 0; i < n; i++) {
      while (nodes[i].after_compaction_ind != static_cast<std::int32_t>(i)) {
        auto t = static_cast<std::uint32_t>(nodes[i].after_compaction_ind);
        std::swap(nodes[i], nodes[t]);
      }
    }

    // ── Phase 5: update handle indices & reset scratch ──
    for (std::uint32_t i = 0; i < n; i++) {
      if (nodes[i].handle)
        nodes[i].handle->index = i;
      nodes[i].after_compaction_ind = -1;
    }
  }
};

} // namespace audioapi::utils::graph
