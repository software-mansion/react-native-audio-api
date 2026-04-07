#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/NodeHandle.h>
#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/Result.hpp>

#include <memory>
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
  using AGEvent = FatFunction<32, void(AudioGraph &, Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> &)>;

  using Res = Result<AGEvent, ResultError>;

  /// Per-node scratch used by graph traversals (e.g. hasPath).
  struct TraversalState {
    size_t term = 0;

    /// @return true if node was not yet visited in the current traversal term
    bool visit(size_t currentTerm);
  };

  /// A single node in the HostGraph.
  struct Node {
    std::vector<Node *> inputs;  // reversed edges
    std::vector<Node *> outputs; // forward edges
    TraversalState traversalState;
    std::shared_ptr<NodeHandle> handle; // shared handle bridging to AudioGraph
    bool ghost = false; // kept for cycle detection until AudioGraph confirms deletion

#if RN_AUDIO_API_TEST
    size_t test_node_identifier__ = 0;
#endif

    /// Destructor tears down all edges touching this node.
    ~Node();
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

  /// @brief Marks a node as processing and recursively marks all inputs as processing.
  static void markNodesAsProcessing(Node *node);

  /// @brief Removes a directed edge from → to.
  /// @return AGEvent that removes the input on the AudioGraph side.
  Res removeEdge(Node *from, Node *to);

  /// @brief Marks a node as not processing and recursively marks all inputs as not processing.
  static void markNodesAsNotProcessing(Node *node);

  /// @brief Removes all outgoing edges from `from`.
  /// @return single AGEvent that removes all inputs on the AudioGraph side, or NODE_NOT_FOUND.
  Res removeAllEdges(Node *from);

  /// @brief Current number of live (non-ghost) edges.
  [[nodiscard]] size_t edgeCount() const;

  /// @brief Current number of nodes (including ghosts).
  [[nodiscard]] size_t nodeCount() const;

 private:
  std::vector<Node *> nodes;
  size_t edgeCount_ = 0;
  size_t last_term = 0; // monotonic counter for traversal freshness

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
