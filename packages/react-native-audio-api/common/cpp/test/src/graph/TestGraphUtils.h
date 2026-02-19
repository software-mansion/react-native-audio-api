#pragma once

#if !RN_AUDIO_API_TEST
#error "RN_AUDIO_API_TEST must be enabled to use TestGraphUtils"
#define RN_AUDIO_API_TEST true // for intellisense
#endif

#include <audioapi/core/utils/graph/AudioGraph.hpp>
#include <audioapi/core/utils/graph/HostGraph.hpp>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

class TestGraphUtils {
 public:
  /// @brief Creates a test graph based on the provided adjacency list.
  /// @param adjacencyList The adjacency list representing the connections between nodes in the graph.
  /// @return A pair of AudioGraph<AudioNode> and HostGraph<AudioNode> representing the created test graph.
  /// It creates a graph based on simple adjacency list where each index corresponds to a node and the vector at that index contains the indices of its input nodes. The function should construct both the AudioGraph<AudioNode> and HostGraph<AudioNode> accordingly, ensuring that the relationships between nodes are correctly established in both graphs.
  static std::pair<AudioGraph<AudioNode>, HostGraph<AudioNode>> createTestGraph(
      std::vector<std::vector<size_t>> adjacencyList);

  /// @brief Converts the given AudioGraph<AudioNode> into an adjacency list representation.
  /// @param audioGraph The AudioGraph<AudioNode> to be converted.
  /// @return An adjacency list representing the connections between nodes in the graph, where each index corresponds
  /// @note for equality checks
  static std::vector<std::vector<size_t>> convertAudioGraphToAdjacencyList(
      const AudioGraph<AudioNode> &audioGraph);

  /// @brief Converts the given HostGraph<AudioNode> into an adjacency list representation.
  /// @param hostGraph The HostGraph<AudioNode> to be converted.
  /// @return An adjacency list representing the connections between nodes in the graph, where each index corresponds to a node and the vector at that index contains the indices of its input nodes.
  /// @note for equality checks
  static std::vector<std::vector<size_t>> convertHostGraphToAdjacencyList(
      const HostGraph<AudioNode> &hostGraph);

 private:
  // Helper function to create a HostGraph<AudioNode> from an adjacency list
  static HostGraph<AudioNode> makeFromAdjacencyList(
      const std::vector<std::vector<size_t>> &adjacencyList);

  // Helper function to create an AudioGraph<AudioNode> from a HostGraph<AudioNode>
  static AudioGraph<AudioNode> createAudioGraphFromHostGraph(const HostGraph<AudioNode> &hostGraph);
};

} // namespace audioapi::utils::graph
