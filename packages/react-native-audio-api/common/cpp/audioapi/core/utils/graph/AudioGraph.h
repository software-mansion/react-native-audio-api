#pragma once
#include <concepts>
#include <memory>
#include <vector>
#if RN_AUDIO_API_TEST
#include <gtest/gtest_prod.h>
#endif // RN_AUDIO_API_TEST

namespace audioapi::utils::graph {

// Forward declarations
#if RN_AUDIO_API_TEST
class AudioGraphTest;
#endif // RN_AUDIO_API_TEST
class HostGraph;

class AudioGraph {
 public:
  struct Node {
    // std::unique_ptr<AudioNode> audioNode;
    std::vector<Node *> inputs;

    Node *next = nullptr;        // next in topological order
    Node *prev = nullptr;        // previous in topological order
    size_t topologicalIndex = 0; // for swapping

#if RN_AUDIO_API_TEST
    // Identifier for testing purposes only
    size_t test_node_identifier__ = 0;
#endif // RN_AUDIO_API_TEST
  };

  struct Iterator {
    explicit Iterator(Node *start) : current(start) {}
    ~Iterator() = default;
    Node *next();

   private:
    Node *current = nullptr;
  };

  AudioGraph() = default;

  /// @brief Destructor that cleans up all nodes in the graph
  /// @note Graph owns all of its nodes so they are deleted here
  ~AudioGraph();

 private:
  Node *head = nullptr;

  void swapNodesInTopologicalOrder(Node *nodeA, Node *nodeB);

// Granting access
#if RN_AUDIO_API_TEST
  friend class AudioGraphTest;
#endif // RN_AUDIO_API_TEST
  friend class HostGraph;
};

} // namespace audioapi::utils::graph
