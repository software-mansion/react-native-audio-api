#pragma once

#include <audioapi/core/utils/graph/AudioGraph.h>
#include <audioapi/core/utils/graph/Disposer.hpp>
#include <audioapi/utils/DSU.hpp>
#include <audioapi/utils/FatFunction.hpp>

#include <utility>
#include <vector>

namespace audioapi::utils::graph {

class HostGraph;
class TestGraphUtils;

/// @brief HostGraph is responsible for managing the graph structure, including adding/removing nodes and edges, and maintaining the topological order. It should also provide methods for traversing the graph and accessing node information. The HostGraph will interact with the AudioGraph to ensure that the structure of the graph is maintained correctly, and it will handle any necessary updates to the AudioGraph when changes are made to the graph structure in the HostGraph.
/// @note It is intended to be used with caution as it does not give any safety guarantees about consistency and does not check correctness of the AudioGraph referenced in events. It also does not check if Node pointers haven't been deallocated in AudioGraph. So there are a lot of assumptions and please use wrapper
/// @note It is izomorphic to AudioGraph in terms of nodes and edges, but it also maintains additional data for faster operations
class HostGraph {
 public:
  using AGEvent = FatFunction<32, void(AudioGraph&, Disposer&)>; // Event that modifies AudioGraph to keep it consistent with HostGraph changes
  struct TraversalState {
    size_t term = 0; // for classification of temp data as old or new

    /// @brief Visits a node during traversal, marking it as visited and updating the term. This function can be used to track the traversal state of nodes in the graph, allowing for algorithms such as depth-first search (DFS) or breadth-first search (BFS) to be implemented effectively.
    /// @param currentTerm The current traversal term to mark the node with.
    /// @return true if node was not visited in the current traversal (term), false otherwise
    bool visit(size_t currentTerm);
  };

  struct Node {
    std::vector<Node *> inputs;            // reversed edges
    std::vector<Node *> outputs;           // edges
    TraversalState traversalState; // for graph traversals
    uint32_t audioNodeIndex = 0; // index of the corresponding node in AudioGraph

#if RN_AUDIO_API_TEST
    // Identifier for testing purposes only
    size_t test_node_identifier__ = 0;
#endif // RN_AUDIO_API_TEST

    /// @brief Destructor cleans up all edges connected to this node. It removes this node from the inputs and outputs of its neighboring nodes.
    /// @note it does NOT destroy corresponding AudioGraph::Node
    ~Node();
  };

  HostGraph();
  ~HostGraph();

  HostGraph(const HostGraph &) = delete;
  HostGraph &operator=(const HostGraph &) = delete;

  HostGraph(HostGraph &&other) noexcept;
  HostGraph &operator=(HostGraph &&other) noexcept;

  /// @brief Adds a new node to the graph.
  /// @param audioNodeIndex Index of the AudioGraph::Node.
  std::pair<Node *, AGEvent> addNode(uint32_t audioNodeIndex);

  /// @brief Removes a node from the graph.
  AGEvent removeNode(Node *node);

  /// @brief Adds an edge. Checks for cycles using DFS.
  /// @return Event or empty if cycle detected.
  AGEvent addEdge(Node *from, Node *to);

  /// @brief Removes an edge.
  AGEvent removeEdge(Node *from, Node *to);

 private:
  // We own the nodes now
  std::vector<Node*> nodes;
  size_t last_term = 0; // for traversal data management

  bool hasPath(Node* from, Node* to);

  friend class TestGraphUtils;
  friend class HostGraphTest;
};

} // namespace audioapi::utils::graph
