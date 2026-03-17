#include "TestGraphUtils.h"

#include <algorithm>
#include <map>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

std::pair<AudioGraph<MockNode>, HostGraph<MockNode>> TestGraphUtils::createTestGraph(
    std::vector<std::vector<size_t>> adjacencyList) {
  HostGraph<MockNode> hostGraph = makeFromAdjacencyList(adjacencyList);
  AudioGraph<MockNode> audioGraph = createAudioGraphFromHostGraph(hostGraph);
  return {std::move(audioGraph), std::move(hostGraph)};
}

std::vector<std::vector<size_t>> TestGraphUtils::convertAudioGraphToAdjacencyList(
    const AudioGraph<MockNode> &audioGraph) {
  std::vector<std::vector<size_t>> adjacencyList;
  if (audioGraph.size() == 0)
    return {};

  size_t maxId = 0;
  for (uint32_t i = 0; i < audioGraph.size(); i++) {
    if (audioGraph[i].test_node_identifier__ > maxId) {
      maxId = audioGraph[i].test_node_identifier__;
    }
  }

  adjacencyList.resize(maxId + 1);

  for (uint32_t i = 0; i < audioGraph.size(); i++) {
    const auto &node = audioGraph[i];
    size_t nodeId = node.test_node_identifier__;

    for (uint32_t inputIdx : audioGraph.pool().view(node.input_head)) {
      if (inputIdx < audioGraph.size()) {
        size_t inputId = audioGraph[inputIdx].test_node_identifier__;
        adjacencyList[inputId].push_back(nodeId);
      }
    }
  }

  for (auto &adj : adjacencyList) {
    std::sort(adj.begin(), adj.end());
  }

  return adjacencyList;
}

std::vector<std::vector<size_t>> TestGraphUtils::convertHostGraphToAdjacencyList(
    const HostGraph<MockNode> &hostGraph) {
  std::vector<std::vector<size_t>> adjacencyList;
  if (hostGraph.nodes.empty())
    return {};

  size_t maxId = 0;
  for (auto *n : hostGraph.nodes) {
    if (n->test_node_identifier__ > maxId) {
      maxId = n->test_node_identifier__;
    }
  }

  adjacencyList.resize(maxId + 1);

  for (auto *n : hostGraph.nodes) {
    size_t nodeId = n->test_node_identifier__;
    for (HostGraph<MockNode>::Node *output : n->outputs) {
      if (output) {
        adjacencyList[nodeId].push_back(output->test_node_identifier__);
      }
    }
    std::sort(adjacencyList[nodeId].begin(), adjacencyList[nodeId].end());
  }

  return adjacencyList;
}

HostGraph<MockNode> TestGraphUtils::makeFromAdjacencyList(
    const std::vector<std::vector<size_t>> &adjacencyList) {
  HostGraph<MockNode> graph;
  std::vector<HostGraph<MockNode>::Node *> nodesVec;
  nodesVec.reserve(adjacencyList.size());

  for (size_t i = 0; i < adjacencyList.size(); ++i) {
    auto handle = std::make_shared<NodeHandle<MockNode>>(static_cast<uint32_t>(i), nullptr);
    auto *node = new HostGraph<MockNode>::Node();
    node->handle = handle;
    node->test_node_identifier__ = i;
    nodesVec.push_back(node);
    graph.nodes.push_back(node);
  }

  for (size_t fromIndex = 0; fromIndex < adjacencyList.size(); ++fromIndex) {
    for (size_t toIndex : adjacencyList[fromIndex]) {
      if (fromIndex < nodesVec.size() && toIndex < nodesVec.size()) {
        HostGraph<MockNode>::Node *fromNode = nodesVec[fromIndex];
        HostGraph<MockNode>::Node *toNode = nodesVec[toIndex];
        fromNode->outputs.push_back(toNode);
        toNode->inputs.push_back(fromNode);
      }
    }
  }

  graph.last_term = 1;
  return graph;
}

AudioGraph<MockNode> TestGraphUtils::createAudioGraphFromHostGraph(
    const HostGraph<MockNode> &hostGraph) {
  AudioGraph<MockNode> audioGraph;
  if (hostGraph.nodes.empty())
    return audioGraph;

  for (auto *n : hostGraph.nodes) {
    audioGraph.addNode(n->handle);
  }

  for (auto *n : hostGraph.nodes) {
    uint32_t idx = n->handle->index;
    audioGraph[idx].test_node_identifier__ = n->test_node_identifier__;

    audioGraph.pool().freeAll(audioGraph[idx].input_head);
    for (HostGraph<MockNode>::Node *input : n->inputs) {
      audioGraph.pool().push(audioGraph[idx].input_head, input->handle->index);
    }
  }

  audioGraph.markDirty();
  return audioGraph;
}

} // namespace audioapi::utils::graph
