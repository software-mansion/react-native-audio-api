#pragma once

#if !RN_AUDIO_API_TEST
#error "RN_AUDIO_API_TEST must be enabled to use TestGraphUtils"
#define RN_AUDIO_API_TEST true // for intellisense
#endif

#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

class TestGraphUtils {
 public:
  /// @brief Creates a test graph based on the provided adjacency list.
  /// @param adjacencyList The adjacency list representing the connections between nodes in the graph.
  /// @return A pair of AudioGraph and HostGraph representing the created test graph.
  /// It creates a graph based on simple adjacency list where each index corresponds to a node and the vector at that index contains the indices of its input nodes. The function should construct both the AudioGraph and HostGraph accordingly, ensuring that the relationships between nodes are correctly established in both graphs.
  static std::pair<AudioGraph, HostGraph> createTestGraph(
      std::vector<std::vector<size_t>> adjacencyList);

  /// @brief Converts the given AudioGraph into an adjacency list representation.
  /// @param audioGraph The AudioGraph to be converted.
  /// @return An adjacency list representing the connections between nodes in the graph, where each index corresponds
  /// @note for equality checks
  static std::vector<std::vector<size_t>> convertAudioGraphToAdjacencyList(
      const AudioGraph &audioGraph);

  /// @brief Converts the given HostGraph into an adjacency list representation.
  /// @param hostGraph The HostGraph to be converted.
  /// @return An adjacency list representing the connections between nodes in the graph, where each index corresponds to a node and the vector at that index contains the indices of its input nodes.
  /// @note for equality checks
  static std::vector<std::vector<size_t>> convertHostGraphToAdjacencyList(
      const HostGraph &hostGraph);

 private:
  // Helper function to create a HostGraph from an adjacency list
  static HostGraph makeFromAdjacencyList(const std::vector<std::vector<size_t>> &adjacencyList);

  // Helper function to create an AudioGraph from a HostGraph
  static AudioGraph createAudioGraphFromHostGraph(const HostGraph &hostGraph);
};

} // namespace audioapi::utils::graph
