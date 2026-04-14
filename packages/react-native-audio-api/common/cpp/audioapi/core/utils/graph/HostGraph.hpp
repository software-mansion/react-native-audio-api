#pragma once

#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/Disposer.hpp>
#include <audioapi/core/utils/graph/NodeHandle.hpp>
#include <audioapi/utils/FatFunction.hpp>
#include <audioapi/utils/Result.hpp>

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

class GraphCycleDebugTest;

namespace audioapi::utils::graph {

template <AudioGraphNode NodeType>
class HostGraph;
template <AudioGraphNode NodeType>
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
template <AudioGraphNode NodeType>
class HostGraph {
 public:
  enum class ResultError {
    NODE_NOT_FOUND,
    CYCLE_DETECTED,
    EDGE_NOT_FOUND,
    EDGE_ALREADY_EXISTS,
  };

  /// Size of the Disposer payload (= sizeof(std::unique_ptr<T[]>)).
  static constexpr size_t kDisposerPayloadSize = 8;

  /// Event that modifies AudioGraph to keep it consistent with HostGraph.
  /// The second argument is the Disposer used to offload buffer deallocation.
  using AGEvent = FatFunction<32, void(AudioGraph<NodeType> &, Disposer<kDisposerPayloadSize> &)>;

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
    std::shared_ptr<NodeHandle<NodeType>> handle; // shared handle bridging to AudioGraph
    bool ghost = false; // kept for cycle detection until AudioGraph confirms deletion

#if RN_AUDIO_API_TEST
    size_t test_node_identifier__ = 0;
#endif

    /// Destructor tears down all edges touching this node.
    ~Node();
  };

  // ── Lifecycle ───────────────────────────────────────────────────────────

  HostGraph() = default;
  ~HostGraph();

  HostGraph(const HostGraph &) = delete;
  HostGraph &operator=(const HostGraph &) = delete;

  HostGraph(HostGraph &&other) noexcept;
  HostGraph &operator=(HostGraph &&other) noexcept;

  // ── Public API ──────────────────────────────────────────────────────────

  /// @brief Adds a new node to the graph.
  /// @param handle shared handle that bridges HostGraph ↔ AudioGraph
  /// @return pair of (raw Node pointer, AGEvent to replay on AudioGraph)
  std::pair<Node *, AGEvent> addNode(std::shared_ptr<NodeHandle<NodeType>> handle);

  /// @brief Removes a node (marks it as ghost, keeps edges for cycle detection).
  /// @return AGEvent that sets `orphaned = true` on the AudioGraph side.
  Res removeNode(Node *node);

  /// @brief Adds a directed edge from → to. Rejects cycles and duplicates.
  /// @return AGEvent that adds the input on the AudioGraph side.
  Res addEdge(Node *from, Node *to);

  /// @brief Removes a directed edge from → to.
  /// @return AGEvent that removes the input on the AudioGraph side.
  Res removeEdge(Node *from, Node *to);

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

  friend class Graph<NodeType>;
  friend class TestGraphUtils;
  friend class HostGraphTest;
  friend class GraphCycleDebugTest;
};

// =========================================================================
// Implementation
// =========================================================================

template <AudioGraphNode NodeType>
bool HostGraph<NodeType>::TraversalState::visit(size_t currentTerm) {
  if (term == currentTerm) {
    return false;
  }
  term = currentTerm;
  return true;
}

template <AudioGraphNode NodeType>
HostGraph<NodeType>::Node::~Node() {
  for (Node *input : inputs) {
    auto &outs = input->outputs;
    outs.erase(std::remove(outs.begin(), outs.end(), this), outs.end());
  }
  for (Node *output : outputs) {
    auto &inps = output->inputs;
    inps.erase(std::remove(inps.begin(), inps.end(), this), inps.end());
  }
}

// ── Lifecycle ─────────────────────────────────────────────────────────────

template <AudioGraphNode NodeType>
HostGraph<NodeType>::~HostGraph() {
  for (Node *n : nodes) {
    delete n;
  }
  nodes.clear();
}

template <AudioGraphNode NodeType>
HostGraph<NodeType>::HostGraph(HostGraph &&other) noexcept
    : nodes(std::move(other.nodes)), edgeCount_(other.edgeCount_), last_term(other.last_term) {
  other.edgeCount_ = 0;
  other.last_term = 0;
}

template <AudioGraphNode NodeType>
auto HostGraph<NodeType>::operator=(HostGraph &&other) noexcept -> HostGraph & {
  if (this != &other) {
    for (Node *n : nodes) {
      delete n;
    }
    nodes = std::move(other.nodes);
    edgeCount_ = other.edgeCount_;
    last_term = other.last_term;
    other.edgeCount_ = 0;
    other.last_term = 0;
  }
  return *this;
}

template <AudioGraphNode NodeType>
auto HostGraph<NodeType>::addNode(std::shared_ptr<NodeHandle<NodeType>> handle)
    -> std::pair<Node *, AGEvent> {
  Node *newNode = new Node();
  newNode->handle = handle;
  nodes.push_back(newNode);

  auto event = [h = std::move(handle)](auto &graph, auto &) {
    graph.addNode(h);
  };

  return {newNode, std::move(event)};
}

template <AudioGraphNode NodeType>
auto HostGraph<NodeType>::removeNode(Node *node) -> Res {
  auto it = std::find(nodes.begin(), nodes.end(), node);
  if (it == nodes.end()) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  node->ghost = true;

  return Res::Ok(
      [h = node->handle](AudioGraph<NodeType> &graph, auto &) { graph[h->index].orphaned = true; });
}

template <AudioGraphNode NodeType>
auto HostGraph<NodeType>::addEdge(Node *from, Node *to) -> Res {
  if (std::find(nodes.begin(), nodes.end(), from) == nodes.end() ||
      std::find(nodes.begin(), nodes.end(), to) == nodes.end()) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  for (Node *out : from->outputs) {
    if (out == to)
      return Res::Err(ResultError::EDGE_ALREADY_EXISTS);
  }

  if (hasPath(to, from)) {
    return Res::Err(ResultError::CYCLE_DETECTED);
  }

  from->outputs.push_back(to);
  to->inputs.push_back(from);
  edgeCount_++;

  return Res::Ok([hFrom = from->handle, hTo = to->handle](AudioGraph<NodeType> &graph, auto &) {
    graph.pool().push(graph[hTo->index].input_head, hFrom->index);
    graph.markDirty();
  });
}

template <AudioGraphNode NodeType>
auto HostGraph<NodeType>::removeEdge(Node *from, Node *to) -> Res {
  if (std::find(nodes.begin(), nodes.end(), from) == nodes.end() ||
      std::find(nodes.begin(), nodes.end(), to) == nodes.end()) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  auto itOut = std::find(from->outputs.begin(), from->outputs.end(), to);
  if (itOut == from->outputs.end()) {
    return Res::Err(ResultError::EDGE_NOT_FOUND);
  }

  auto itIn = std::find(to->inputs.begin(), to->inputs.end(), from);
  if (itIn != to->inputs.end()) {
    to->inputs.erase(itIn);
  }
  from->outputs.erase(itOut);
  edgeCount_--;

  return Res::Ok([hFrom = from->handle, hTo = to->handle](AudioGraph<NodeType> &graph, auto &) {
    graph.pool().remove(graph[hTo->index].input_head, hFrom->index);
    graph.markDirty();
  });
}

template <AudioGraphNode NodeType>
bool HostGraph<NodeType>::hasPath(Node *start, Node *end) {
  if (start == end) {
    return true;
  }

  last_term++;
  size_t term = last_term;

  std::vector<Node *> stack;
  stack.push_back(start);
  start->traversalState.term = term;

  while (!stack.empty()) {
    Node *curr = stack.back();
    stack.pop_back();

    if (curr == end) {
      return true;
    }

    for (Node *out : curr->outputs) {
      if (out->traversalState.visit(term)) {
        stack.push_back(out);
      }
    }
  }
  return false;
}

template <AudioGraphNode NodeType>
size_t HostGraph<NodeType>::edgeCount() const {
  return edgeCount_;
}

template <AudioGraphNode NodeType>
size_t HostGraph<NodeType>::nodeCount() const {
  return nodes.size();
}

template <AudioGraphNode NodeType>
void HostGraph<NodeType>::collectDisposedNodes() {
  for (auto it = nodes.begin(); it != nodes.end();) {
    Node *n = *it;
    if (n->ghost && n->handle.use_count() == 1) {
      // ~Node() tears down edges from neighbor lists.
      // We decrement for each unique edge (stored once in outputs).
      edgeCount_ -= n->outputs.size();
      *it = nodes.back();
      nodes.pop_back();
      delete n;
    } else {
      ++it;
    }
  }
}

} // namespace audioapi::utils::graph
