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
    /// together). One-way: when THIS node is marked (not)processing, every
    /// entry in `linkedNodes` is marked too (recursively).
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
  /// When `from` is marked (not)processing, `to` will be too. The link is
  /// one-way and does NOT create a graph edge (no AudioGraph side effect).
  /// Intended for host nodes that share processing semantics but do not
  /// share an audio edge (e.g. DelayReader → DelayWriter).
  ///
  /// If the same pair is already linked, this is a no-op.
  static void linkNodes(Node *from, Node *to);

  /// @brief Marks this node CONDITIONAL_PROCESSABLE, then recurses into inputs
  /// and linkedNodes. Skips nodes that are already processable (terminates
  /// cycles and redundant paths).
  static void markNodesAsProcessing(Node *node);

  /// @brief Removes a directed edge from → to.
  /// @return AGEvent that removes the input on the AudioGraph side.
  Res removeEdge(Node *from, Node *to);

  /// @brief Marks a node as not processing and recursively marks all inputs as not processing.
  static void markNodesAsNotProcessing(Node *node);

  /// @brief Audio-thread helper shared by removeEdge / removeAllEdges.
  ///
  /// After `from` loses one or more outputs, tears down its conditional
  /// processing state iff it was only kept processing for a downstream
  /// consumer and none of its remaining outputs still require it. No-op when
  /// `from` is not `CONDITIONAL_PROCESSABLE`. Reads `from->outputs`, so it must
  /// run after the host-side edge removal has been applied.
  static void updateProcessingStateAfterDisconnect(Node *from);

  /// @brief Removes all outgoing edges from `from`.
  /// @return single AGEvent that removes all inputs on the AudioGraph side, or NODE_NOT_FOUND.
  Res removeAllEdges(Node *from);

  /// @brief Current number of live (non-ghost) edges.
  [[nodiscard]] size_t edgeCount() const;

  /// @brief Current number of nodes (including ghosts).
  [[nodiscard]] size_t nodeCount() const;

 private:
  std::vector<Node *> nodes;
  /// Guards access to `nodes` and the per-node adjacency mutated by the
  /// public API (inputs/outputs/ghost). Public API methods do not call one
  /// another while holding the lock, so a plain mutex is sufficient.
  mutable std::mutex nodesMutex_;
  size_t edgeCount_ = 0;
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
