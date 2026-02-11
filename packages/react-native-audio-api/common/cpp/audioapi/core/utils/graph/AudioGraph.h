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
    // std::unique_ptr<AudioNode> audioNode; // The actual audio node data, managed externally by HostGraph events
    std::vector<uint32_t> inputs; // indices of input nodes
    uint32_t topo_out_degree = 0; // scratch space for topological sort (used as Out-Degree counter)

    // Memory pool optimization:
    // -1 means the slot is taken (node is active).
    // otherwise, it points to the next free slot index.
    // if equal to nodes.size(), it is the last free slot.
    int32_t next_free_slot = -1;

#if RN_AUDIO_API_TEST
    // Identifier for testing purposes only
    size_t test_node_identifier__ = 0;
#endif // RN_AUDIO_API_TEST

    bool isActive() const {
      return next_free_slot == -1;
    }
  };

  AudioGraph();
  ~AudioGraph() = default;

  AudioGraph(const AudioGraph &) = delete;
  AudioGraph &operator=(const AudioGraph &) = delete;

  AudioGraph(AudioGraph &&other) noexcept;
  AudioGraph &operator=(AudioGraph &&other) noexcept;

  // The main storage. Be careful with pointer invalidation if resizing.
  std::vector<Node> nodes;
  std::vector<uint32_t> executionOrder;

  // Points to the first free slot in the `nodes` vector.
  // If equal to nodes.size(), it means there are no free slots (and we should append).
  // Implicitly initialized to 0 (logical empty).
  int32_t first_free_slot = 0;

  // Helpers
  void markDirty() {
    isDirty = true;
  }
  void process(); // Recomputes topo order if dirty

  // Allocator helpers
  uint32_t createNode();
  void releaseNode(uint32_t index);

 private:
  bool isDirty = false;
  friend class HostGraph;
  friend class TestGraphUtils;

  void recomputeTopologicalOrder();
};

} // namespace audioapi::utils::graph
