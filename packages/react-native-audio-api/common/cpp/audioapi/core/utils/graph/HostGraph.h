#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/Result.hpp>

#include <audioapi/utils/Macros.h>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

class GraphCycleDebugTest;

namespace audioapi::utils::graph {

class Graph;
class TestGraphUtils;

/// @brief Main-thread graph mirror that keeps structure in sync with AudioGraph.
///
/// Maintains adjacency lists (inputs / outputs) for O(V+E) cycle detection
/// via DFS. Every mutation produces an `AGEvent` lambda that, when executed on
/// the audio thread, applies the same structural change to AudioGraph.
///
/// Ghost nodes: when a node is removed it is marked `ghost = true` but its
/// edges are kept so that `hasPath` still sees paths through nodes that are
/// alive in AudioGraph. Ghosts are collected once AudioGraph releases its
/// shared_ptr (detected via `use_count() == 1`).
///
/// @note Use through the Graph wrapper for safety.
class HostGraph {
 public:
  enum class GraphError : uint8_t {
    NODE_NOT_FOUND,
    CYCLE_DETECTED,
    EDGE_NOT_FOUND,
    EDGE_ALREADY_EXISTS,
  };

  /// Event that modifies AudioGraph to keep it consistent with HostGraph.
  /// The second argument is the Disposer used to offload buffer deallocation.
  using AGEvent = FatFunction<
      AUDIO_GRAPH_EVENT_SIZE,
      void(AudioGraph &, Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> &)>;

  /// Per-node scratch used by graph traversals (e.g. hasPath).
  struct TraversalState {
    size_t term = 0;

    /// @return true if node was not yet visited in the current traversal term
    bool visit(size_t currentTerm);
  };

  /// A single node in the HostGraph.
  struct HostVertex {
    std::vector<HostVertex *> inputs;  // reversed edges
    std::vector<HostVertex *> outputs; // forward edges
    TraversalState traversalState;
    /// Host-side copy of `GraphObject::breaksCycle_`, as last sent to AudioGraph.
    bool breaksCycle = false;
    /// Visited stamp for channel-negotiation passes; `HostGraph::channelLayoutTerm_`
    /// is bumped once per structural change.
    TraversalState channelLayoutTraversal;
    /// Width of the last negotiated buffer queued for the audio thread, seeded
    /// in `addNode` from the buffer built at construction (host thread only).
    /// Negotiation compares against this, not the live buffer: the live one
    /// still lacks swaps queued earlier in the same render quantum.
    size_t pendingChannelCount = 0;
    std::shared_ptr<NodeHandle> handle; // shared handle bridging to AudioGraph
    bool ghost = false; // kept for cycle detection until AudioGraph confirms deletion

#if RN_AUDIO_API_TEST
    size_t test_node_identifier__ = 0;
#endif

    /// Destructor tears down all edges touching this node.
    ~HostVertex();
    HostVertex() = default;
    DELETE_COPY_AND_MOVE(HostVertex);
  };

  // ── Lifecycle ───────────────────────────────────────────────────────────

  HostGraph();
  ~HostGraph();

  HostGraph(const HostGraph &) = delete;
  HostGraph &operator=(const HostGraph &) = delete;

  HostGraph(HostGraph &&other) noexcept;
  HostGraph &operator=(HostGraph &&other) noexcept;

  // ── Public API ──────────────────────────────────────────────────────────

  /// @brief Adds a new node to the graph.
  /// @param handle shared handle that bridges HostGraph ↔ AudioGraph
  /// @return pair of (raw HostVertex pointer, AGEvent to replay on AudioGraph)
  std::pair<HostVertex *, AGEvent> addNode(std::shared_ptr<NodeHandle> handle);

  /// @brief Removes a node (marks it as ghost, keeps edges for cycle detection).
  /// @return AGEvent that sets `orphaned = true` on the AudioGraph side.
  Result<AGEvent, GraphError> removeNode(HostVertex *node);

  /// @brief Adds a directed edge from → to. Rejects duplicates and every cycle
  /// that runs through no cycle breaker.
  /// @return AGEvent that adds the input on the AudioGraph side.
  Result<AGEvent, GraphError> addEdge(HostVertex *from, HostVertex *to);

  /// @brief Removes a directed edge from → to.
  /// @return AGEvent that removes the input on the AudioGraph side.
  Result<AGEvent, GraphError> removeEdge(HostVertex *from, HostVertex *to);

  /// @brief Removes all outgoing edges from `from`.
  /// @return single AGEvent that removes all inputs on the AudioGraph side, or NODE_NOT_FOUND.
  Result<AGEvent, GraphError> removeAllEdges(HostVertex *from);

  /// @brief Recomputes channel negotiation starting at `node` (and
  /// cascading downstream toward AudioDestinationNode), without any structural
  /// change. Used when a node's `channelCount` / `channelCountMode` attribute
  /// changes after construction. The returned AGEvent applies the negotiated
  /// buffer swaps on the audio thread and marks the graph dirty.
  /// @return AGEvent to replay on AudioGraph, or NODE_NOT_FOUND.
  Result<AGEvent, GraphError> renegotiateNodeChannels(HostVertex *node);

  /// @brief Current number of live (non-ghost) edges.
  [[nodiscard]] size_t edgeCount() const;

  /// @brief Current number of nodes (including ghosts).
  [[nodiscard]] size_t nodeCount() const;

 private:
  std::vector<HostVertex *> nodes;
  /// Guards access to `nodes` and the per-node adjacency mutated by the
  /// public API (inputs/outputs/ghost). Public API methods do not call one
  /// another while holding the lock, so a plain mutex is sufficient.
  mutable std::mutex nodesMutex_;
  size_t edgeCount_ = 0;
  size_t last_term = 0;          // monotonic counter for traversal freshness
  size_t channelLayoutTerm_ = 0; // monotonic counter for channel negotiation passes

  /// @brief DFS reachability check (traverses ghosts too).
  /// @param throughCycleBreakers when false, only paths that visit no cycle
  /// breaker count, `start` and `end` included.
  /// @note JS Thread only
  bool hasPath(HostVertex *start, HostVertex *end, bool throughCycleBreakers);

  /// Nodes whose `GraphObject::breaksCycle_` changed, with the new value.
  using CycleBreakerUpdates = std::vector<std::pair<std::shared_ptr<NodeHandle>, bool>>;

  /// @brief Recomputes which nodes break a cycle: the ones that can and that
  /// lie on one. Call after every edge change and apply the result in that
  /// change's event, so AudioGraph never sees new edges with old values.
  /// @return the nodes whose value changed.
  /// @note JS Thread only
  std::unique_ptr<CycleBreakerUpdates> markCycleBreakers();

  /// @brief Writes `updates` into the graph objects.
  /// @note Audio Thread only
  static void applyCycleBreakerUpdates(const CycleBreakerUpdates &updates);

  /// @brief Scans ghost nodes and deletes those whose handle has
  /// `use_count() == 1`, meaning AudioGraph has released its reference.
  void collectDisposedNodes();

  friend class Graph;
  friend class TestGraphUtils;
  friend class HostGraphTest;
  friend class GraphCycleDebugTest;
};

} // namespace audioapi::utils::graph
