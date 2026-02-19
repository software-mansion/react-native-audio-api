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
  if (audioGraph.size() == 0) return {};

  size_t maxId = 0;
  for (uint32_t i = 0; i < audioGraph.size(); i++) {
    if (audioGraph[i].test_node_identifier__ > maxId) {
      maxId = audioGraph[i].test_node_identifier__;
    }
  }

  adjacencyList.resize(maxId + 1);

  for (uint32_t i = 0; i < audioGraph.size(); i++) {
    const auto& node = audioGraph[i];
    size_t nodeId = node.test_node_identifier__;

    for (uint32_t inputIdx : node.inputs) {
        if (inputIdx < audioGraph.size()) {
            size_t inputId = audioGraph[inputIdx].test_node_identifier__;
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
  std::vector<HostGraph::Node*> nodesVec;
  nodesVec.reserve(adjacencyList.size());

  // Create nodes with shared handles
  for (size_t i = 0; i < adjacencyList.size(); ++i) {
    auto handle = std::make_shared<NodeHandle>(static_cast<uint32_t>(i), nullptr);
    HostGraph::Node* node = new HostGraph::Node();
    node->handle = handle;
    node->test_node_identifier__ = i;
    nodesVec.push_back(node);
    graph.nodes.push_back(node);
  }

  // Create edges: adjacencyList[i] = {j, k} means i -> j, i -> k
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

  graph.last_term = 1;
  return graph;
}

AudioGraph TestGraphUtils::createAudioGraphFromHostGraph(const HostGraph &hostGraph) {
  AudioGraph audioGraph;
  if (hostGraph.nodes.empty()) return audioGraph;

  // Add nodes to AudioGraph using shared handles from HostGraph
  for (auto* n : hostGraph.nodes) {
    audioGraph.addNode(n->handle);
  }

  // Set test identifiers and inputs
  for (auto* n : hostGraph.nodes) {
    uint32_t idx = n->handle->index;
    audioGraph[idx].test_node_identifier__ = n->test_node_identifier__;

    audioGraph[idx].inputs.clear();
    for (HostGraph::Node* input : n->inputs) {
      audioGraph[idx].inputs.push_back(input->handle->index);
    }
  }

  audioGraph.markDirty();
  // Note: no process() call here — toposort is done as part of process(disposer)
  // For tests that need toposort, call audioGraph.process(disposer) explicitly

  return audioGraph;
}

} // namespace audioapi::utils::graph

