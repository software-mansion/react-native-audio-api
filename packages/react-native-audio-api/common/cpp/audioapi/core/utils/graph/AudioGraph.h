#pragma once
#include <concepts>
#include <memory>
#include <vector>

namespace audioapi::utils::graph {

// Forward declarations
class HostGraph;
class AudioGraph;
class TestGraphUtils;

/// @brief AudioGraph is only a structure allowing topological traversal
/// @note it is fully managed by events provided by HostGraph
class AudioGraph {
 public:
  struct Node {
    // std::unique_ptr<AudioNode> audioNode;
    std::vector<Node *> inputs;
    Node *next = nullptr; // next in topological order

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

  AudioGraph();

  AudioGraph(const AudioGraph &) = delete;
  AudioGraph &operator=(const AudioGraph &) = delete;

  AudioGraph(AudioGraph &&other) noexcept;
  AudioGraph &operator=(AudioGraph &&other) noexcept;

  /// @brief Destructor that cleans up all nodes in the graph
  /// @note Graph owns all of its nodes so they are deleted here
  ~AudioGraph();

  Iterator iterator() const {
    return Iterator(head->next);
  }

 private:
  // Head is a dummy node that helps with events execution without worrying about edge case of node being head.
  Node *head = nullptr;

  friend class HostGraph;
  friend class TestGraphUtils;
};

} // namespace audioapi::utils::graph
