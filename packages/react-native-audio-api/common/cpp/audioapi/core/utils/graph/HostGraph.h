#pragma once

#include <audioapi/core/utils/graph/AudioGraph.h>
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
  using AGEvent = FatFunction<32, void()>;

  struct TraversalState {
    size_t term = 0; // for classification of temp data as old or new

    /// @brief Visits a node during traversal, marking it as visited and updating the term. This function can be used to track the traversal state of nodes in the graph, allowing for algorithms such as depth-first search (DFS) or breadth-first search (BFS) to be implemented effectively.
    /// @param currentTerm The current traversal term to mark the node with.
    /// @return true if node was not visited in the current traversal (term), false otherwise
    bool visit(size_t currentTerm);
  };

  struct Node {
    AudioGraph::Node *audioNode = nullptr; // pointer to the corresponding node in AudioGraph
    std::vector<Node *> inputs;            // reversed edges
    std::vector<Node *> outputs;           // edges

    Node *next = nullptr;          // next in topological order
    Node *prev = nullptr;          // previous in topological order
    size_t topologicalIndex = 0;   // for swapping
    TraversalState traversalState; // for graph traversals

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

  /// @brief Adds a new node to the graph, corresponding to the given AudioGraph::Node.
  /// @param audioNode Pointer to the AudioGraph::Node to be added.
  /// @return Pointer to the newly created HostGraph::Node. Returned pointer lifetime is tied to the HostGraph instance.
  /// @return An event that should be applied to corresponding AudioGraph to maintain consistency between graphs. The event is a function that takes an AudioGraph reference and modifies it accordingly (e.g., by adding the corresponding AudioGraph::Node).
  std::pair<Node *, AGEvent> addNode(AudioGraph::Node *audioNode);

  /// @brief Removes a node from the graph.
  /// @param node Pointer to the HostGraph::Node to be removed.
  /// @return A function that, when applied to an AudioGraph, will remove the corresponding AudioGraph::Node. The function is a FatFunction that takes an AudioGraph reference and modifies it accordingly (e.g., by removing the corresponding AudioGraph::Node).
  /// @note The returned function should be applied to the corresponding AudioGraph to maintain consistency between the graphs.
  AGEvent removeNode(Node *node);

  /// @brief Adds an edge from one node to another in the graph.
  /// @param from Pointer to the source HostGraph::Node.
  /// @param to Pointer to the destination HostGraph::Node.
  /// @return A function that, when applied to an AudioGraph, will add the corresponding edge between the AudioGraph::Nodes. The function is a FatFunction that takes an AudioGraph reference and modifies it accordingly (e.g., by adding the corresponding edge between the AudioGraph::Nodes).
  AGEvent addEdge(Node *from, Node *to);

  /// @brief Removes an edge from one node to another in the graph.
  /// @param from Pointer to the source HostGraph::Node.
  /// @param to Pointer to the destination HostGraph::Node.
  /// @return A function that, when applied to an AudioGraph, will remove the corresponding edge between the AudioGraph::Nodes. The function is a FatFunction that takes an AudioGraph reference and modifies it accordingly (e.g., by removing the corresponding edge between the AudioGraph::Nodes).
  AGEvent removeEdge(Node *from, Node *to);

 private:
  std::vector<std::unique_ptr<Node>> nodes; // all nodes in the graph

  // Dummy head and tail are nodes that help with edge cases
  Node *head = nullptr; // head of the topologically sorted list of nodes (dummy head)
  Node *tail = nullptr; // tail of the topologically sorted list of nodes (dummy tail)
  size_t last_term = 0; // for traversal data management

  friend class TestGraphUtils;
};

} // namespace audioapi::utils::graph
