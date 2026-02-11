#include "TestGraphUtils.h"

#include <algorithm>
#include <vector>
#include <utility>
#include <memory>
#include <map>

namespace audioapi::utils::graph {

std::pair<AudioGraph, HostGraph> TestGraphUtils::createTestGraph(std::vector<std::vector<size_t>> adjacencyList) {
  HostGraph hostGraph = makeFromAdjacencyList(adjacencyList);
  AudioGraph audioGraph = createAudioGraphFromHostGraph(hostGraph);
  return {std::move(audioGraph), std::move(hostGraph)};
}

std::vector<std::vector<size_t>> TestGraphUtils::convertAudioGraphToAdjacencyList(const AudioGraph &audioGraph) {
  std::vector<std::vector<size_t>> adjacencyList;
  if (audioGraph.nodes.empty()) return {};

  size_t maxId = 0;
  for (const auto& node : audioGraph.nodes) {
    if (node.test_node_identifier__ > maxId) {
      maxId = node.test_node_identifier__;
    }
  }

  adjacencyList.resize(maxId + 1);

  for (const auto& node : audioGraph.nodes) {
    size_t nodeId = node.test_node_identifier__;

    for (uint32_t inputIdx : node.inputs) {
        if (inputIdx < audioGraph.nodes.size()) {
            size_t inputId = audioGraph.nodes[inputIdx].test_node_identifier__;
            adjacencyList[inputId].push_back(nodeId);
        }
    }
  }

  for(auto& adj : adjacencyList) {
      std::sort(adj.begin(), adj.end());
  }

  return adjacencyList;
}

std::vector<std::vector<size_t>> TestGraphUtils::convertHostGraphToAdjacencyList(const HostGraph &hostGraph) {
  std::vector<std::vector<size_t>> adjacencyList;
  if (hostGraph.nodes.empty()) return {};

  size_t maxId = 0;
  for (auto* n : hostGraph.nodes) {
    if (n->test_node_identifier__ > maxId) {
      maxId = n->test_node_identifier__;
    }
  }

  adjacencyList.resize(maxId + 1);

  for (auto* n : hostGraph.nodes) {
    size_t nodeId = n->test_node_identifier__;
    // HostGraph nodes have `outputs`. Use them directly.
    for (HostGraph::Node* output : n->outputs) {
      if (output) {
        adjacencyList[nodeId].push_back(output->test_node_identifier__);
      }
    }
    std::sort(adjacencyList[nodeId].begin(), adjacencyList[nodeId].end());
  }

  return adjacencyList;
}

HostGraph TestGraphUtils::makeFromAdjacencyList(const std::vector<std::vector<size_t>> &adjacencyList) {
  HostGraph graph;
  // Temporary storage to access nodes by index during construction
  std::vector<HostGraph::Node*> nodesVec;
  nodesVec.reserve(adjacencyList.size());

  // Create nodes
  for (size_t i = 0; i < adjacencyList.size(); ++i) {
    HostGraph::Node* node = new HostGraph::Node();
    node->audioNodeIndex = static_cast<uint32_t>(i); // Assume 1:1 mapping for test graph
    node->test_node_identifier__ = i; // Set test identifier
    nodesVec.push_back(node);
    graph.nodes.push_back(node);
  }

  // Create edges based on adjacency list where index i contains list of OUTPUTS from i
  // adjacencyList[i] = {j, k} means i -> j, i -> k
  for (size_t fromIndex = 0; fromIndex < adjacencyList.size(); ++fromIndex) {
    for (size_t toIndex : adjacencyList[fromIndex]) {
      if (fromIndex < nodesVec.size() && toIndex < nodesVec.size()) {
          HostGraph::Node* fromNode = nodesVec[fromIndex];
          HostGraph::Node* toNode = nodesVec[toIndex];

          fromNode->outputs.push_back(toNode);
          toNode->inputs.push_back(fromNode);
      }
    }
  }

  size_t term = 1; // for retrieval of order

  graph.last_term = term;

  return graph;
}

AudioGraph TestGraphUtils::createAudioGraphFromHostGraph(const HostGraph &hostGraph) {
  AudioGraph audioGraph;

  if (hostGraph.nodes.empty()) return audioGraph;

  // Determine size.
  // Since we assumed audioNodeIndex is valid and 0-based packed in tests:
  size_t maxIdx = 0;
  for (auto* n : hostGraph.nodes) {
      if (n->audioNodeIndex > maxIdx) maxIdx = n->audioNodeIndex;
  }

  audioGraph.nodes.resize(maxIdx + 1);

  // Fill nodes
  for (auto* n : hostGraph.nodes) {
      auto& audioNode = audioGraph.nodes[n->audioNodeIndex];
      audioNode.test_node_identifier__ = n->test_node_identifier__;

      // Inputs
      audioNode.inputs.clear();
      for (HostGraph::Node* input : n->inputs) {
          audioNode.inputs.push_back(input->audioNodeIndex);
      }
  }

  // We should also run process() to build executionOrder?
  // The tests might expect it.
  audioGraph.markDirty();
  audioGraph.process();

  return audioGraph;
}

} // namespace audioapi::utils::graph

