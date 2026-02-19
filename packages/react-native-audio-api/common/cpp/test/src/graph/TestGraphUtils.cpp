#include "TestGraphUtils.h"

#include <algorithm>
#include <vector>
#include <utility>
#include <memory>
#include <map>

namespace audioapi::utils::graph {

std::pair<AudioGraph<AudioNode>, HostGraph<AudioNode>> TestGraphUtils::createTestGraph(std::vector<std::vector<size_t>> adjacencyList) {
  HostGraph<AudioNode> hostGraph = makeFromAdjacencyList(adjacencyList);
  AudioGraph<AudioNode> audioGraph = createAudioGraphFromHostGraph(hostGraph);
  return {std::move(audioGraph), std::move(hostGraph)};
}

std::vector<std::vector<size_t>> TestGraphUtils::convertAudioGraphToAdjacencyList(const AudioGraph<AudioNode> &audioGraph) {
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

std::vector<std::vector<size_t>> TestGraphUtils::convertHostGraphToAdjacencyList(const HostGraph<AudioNode> &hostGraph) {
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
    for (HostGraph<AudioNode>::Node* output : n->outputs) {
      if (output) {
        adjacencyList[nodeId].push_back(output->test_node_identifier__);
      }
    }
    std::sort(adjacencyList[nodeId].begin(), adjacencyList[nodeId].end());
  }

  return adjacencyList;
}

HostGraph<AudioNode> TestGraphUtils::makeFromAdjacencyList(const std::vector<std::vector<size_t>> &adjacencyList) {
  HostGraph<AudioNode> graph;
  std::vector<HostGraph<AudioNode>::Node*> nodesVec;
  nodesVec.reserve(adjacencyList.size());

  // Create nodes with shared handles
  for (size_t i = 0; i < adjacencyList.size(); ++i) {
    auto handle = std::make_shared<NodeHandle<AudioNode>>(static_cast<uint32_t>(i), nullptr);
    HostGraph<AudioNode>::Node* node = new HostGraph<AudioNode>::Node();
    node->handle = handle;
    node->test_node_identifier__ = i;
    nodesVec.push_back(node);
    graph.nodes.push_back(node);
  }

  // Create edges: adjacencyList[i] = {j, k} means i -> j, i -> k
  for (size_t fromIndex = 0; fromIndex < adjacencyList.size(); ++fromIndex) {
    for (size_t toIndex : adjacencyList[fromIndex]) {
      if (fromIndex < nodesVec.size() && toIndex < nodesVec.size()) {
          HostGraph<AudioNode>::Node* fromNode = nodesVec[fromIndex];
          HostGraph<AudioNode>::Node* toNode = nodesVec[toIndex];
          fromNode->outputs.push_back(toNode);
          toNode->inputs.push_back(fromNode);
      }
    }
  }

  graph.last_term = 1;
  return graph;
}

AudioGraph<AudioNode> TestGraphUtils::createAudioGraphFromHostGraph(const HostGraph<AudioNode> &hostGraph) {
  AudioGraph<AudioNode> audioGraph;
  if (hostGraph.nodes.empty()) return audioGraph;

  // Add nodes to AudioGraph<AudioNode> using shared handles from HostGraph<AudioNode>
  for (auto* n : hostGraph.nodes) {
    audioGraph.addNode(n->handle);
  }

  // Set test identifiers and inputs
  for (auto* n : hostGraph.nodes) {
    uint32_t idx = n->handle->index;
    audioGraph[idx].test_node_identifier__ = n->test_node_identifier__;

    audioGraph[idx].inputs.clear();
    for (HostGraph<AudioNode>::Node* input : n->inputs) {
      audioGraph[idx].inputs.push_back(input->handle->index);
    }
  }

  audioGraph.markDirty();
  // Note: no process() call here — toposort is done as part of process(disposer)
  // For tests that need toposort, call audioGraph.process(disposer) explicitly

  return audioGraph;
}

} // namespace audioapi::utils::graph

