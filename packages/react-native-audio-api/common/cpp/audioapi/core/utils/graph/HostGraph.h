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
  enum class ResultError : uint8_t {
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

  using Res = Result<AGEvent, ResultError>;

  /// Per-node scratch used by graph traversals (e.g. hasPath).
  struct TraversalState {
    size_t term = 0;

    /// @return true if node was not yet visited in the current traversal term
    bool visit(size_t currentTerm);
  };

  /// Per-node scratch for channel-layout negotiation (host thread only).
  ///
  /// Follows the same generation-stamp pattern as `TraversalState`: each
  /// `addEdge` / `removeEdge` bumps `HostGraph::channelLayoutTerm_`, and nodes
  /// visited in that pass stamp `term` with the current value. A node is
  /// considered resolved for the pass when `term == channelLayoutTerm_`.
  ///
  /// During the pass, `upstreamChannelCount` stores how many channels this
  /// node will present on upstream connections (toward AudioDestinationNode)
  /// after negotiation — including overrides such as StereoPanner's fixed
  /// stereo output. Downstream nodes read these pending widths from their
  /// inputs instead of live output buffers, so multiple connects in one batch
  /// (before the AGEvent applies buffer swaps on the audio thread) still see
  /// consistent layouts. Values are never read on the audio thread.
  struct ChannelLayoutState {
    size_t term = 0;
    size_t upstreamChannelCount = 0;

    [[nodiscard]] bool isResolvedFor(size_t currentTerm) const {
      return term == currentTerm;
    }

    void setResolved(size_t currentTerm, size_t count) {
      term = currentTerm;
      upstreamChannelCount = count;
    }
  };

  /// A single node in the HostGraph.
  struct Node {
    std::vector<Node *> inputs;  // reversed edges
    std::vector<Node *> outputs; // forward edges
    /// Nodes whose processable state should follow this node's state.
    /// Used to tie together the state of logically-linked host nodes that do
    /// not share a graph edge (e.g. DelayReader → DelayWriter communicate via
    /// a ring buffer, so no audio edge exists, but they must be processed
    /// together). The actual processable propagation happens on the audio
    /// thread in AudioGraph::settleProcessableState() via the mirrored
    /// `link_head` entries; this host-side list only exists so links can be
    /// scrubbed when a linked node is disposed.
    std::vector<Node *> linkedNodes;
    TraversalState traversalState;
    ChannelLayoutState channelLayout;
    std::shared_ptr<NodeHandle> handle; // shared handle bridging to AudioGraph
    bool ghost = false; // kept for cycle detection until AudioGraph confirms deletion

#if RN_AUDIO_API_TEST
    size_t test_node_identifier__ = 0;
#endif

    /// Destructor tears down all edges touching this node.
    ~Node();
    Node() = default;
    DELETE_COPY_AND_MOVE(Node);
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
  /// @return pair of (raw Node pointer, AGEvent to replay on AudioGraph)
  std::pair<Node *, AGEvent> addNode(std::shared_ptr<NodeHandle> handle);

  /// @brief Removes a node (marks it as ghost, keeps edges for cycle detection).
  /// @return AGEvent that sets `orphaned = true` on the AudioGraph side.
  Res removeNode(Node *node);

  /// @brief Adds a directed edge from → to. Rejects cycles and duplicates.
  /// @return AGEvent that adds the input on the AudioGraph side.
  Res addEdge(Node *from, Node *to);

  /// @brief Links the processable-state of `from` to propagate into `to`.
  ///
  /// Records the link on `from->linkedNodes` (host-side, for cleanup) and
  /// returns an AGEvent that mirrors it onto the audio graph as a `link_head`
  /// entry. The actual propagation happens on the audio thread in
  /// AudioGraph::settleProcessableState(). One-way and does NOT create a graph
  /// edge. Intended for host nodes that share processing semantics but not an
  /// audio edge (e.g. DelayReader → DelayWriter).
  ///
  /// @return AGEvent to replay on AudioGraph, or std::nullopt if the pair is
  ///         invalid or already linked.
  std::optional<AGEvent> linkNodes(Node *from, Node *to);

  /// @brief Removes a directed edge from → to.
  /// @return AGEvent that removes the input on the AudioGraph side.
  Res removeEdge(Node *from, Node *to);

  /// @brief Removes all outgoing edges from `from`.
  /// @return single AGEvent that removes all inputs on the AudioGraph side, or NODE_NOT_FOUND.
  Res removeAllEdges(Node *from);

  /// @brief Recomputes channel-count negotiation starting at `node` (and
  /// cascading downstream toward AudioDestinationNode), without any structural
  /// change. Used when a node's `channelCount` / `channelCountMode` attribute
  /// changes after construction. The returned AGEvent applies the negotiated
  /// buffer swaps on the audio thread and marks the graph dirty.
  /// @return AGEvent to replay on AudioGraph, or NODE_NOT_FOUND.
  Res renegotiateNodeChannels(Node *node);

  /// @brief Current number of live (non-ghost) edges.
  [[nodiscard]] size_t edgeCount() const;

  /// @brief Current number of processable-links (backs pool slots too).
  [[nodiscard]] size_t linkCount() const;

  /// @brief Current number of nodes (including ghosts).
  [[nodiscard]] size_t nodeCount() const;

 private:
  std::vector<Node *> nodes;
  /// Guards access to `nodes` and the per-node adjacency mutated by the
  /// public API (inputs/outputs/ghost). Public API methods do not call one
  /// another while holding the lock, so a plain mutex is sufficient.
  mutable std::mutex nodesMutex_;
  size_t edgeCount_ = 0;
  size_t linkCount_ = 0;
  size_t last_term = 0;          // monotonic counter for traversal freshness
  size_t channelLayoutTerm_ = 0; // monotonic counter for channel negotiation passes

  /// @brief DFS reachability check (traverses ghosts too).
  bool hasPath(Node *start, Node *end);

  /// @brief Scans ghost nodes and deletes those whose handle has
  /// `use_count() == 1`, meaning AudioGraph has released its reference.
  void collectDisposedNodes();

  friend class Graph;
  friend class TestGraphUtils;
  friend class HostGraphTest;
  friend class GraphCycleDebugTest;
};

} // namespace audioapi::utils::graph
