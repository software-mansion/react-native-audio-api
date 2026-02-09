#include "TestGraphUtils.h"

#include <algorithm>
#include <vector>
#include <utility>
#include <memory>

namespace audioapi::utils::graph {

std::pair<AudioGraph, HostGraph> TestGraphUtils::createTestGraph(std::vector<std::vector<size_t>> adjacencyList) {
  HostGraph hostGraph = makeFromAdjacencyList(adjacencyList);
  AudioGraph audioGraph = createAudioGraphFromHostGraph(hostGraph);
  return {std::move(audioGraph), std::move(hostGraph)};
}

std::vector<std::vector<size_t>> TestGraphUtils::convertAudioGraphToAdjacencyList(const AudioGraph &audioGraph) {
  std::vector<std::vector<size_t>> adjacencyList;

  // First pass: verify we can use test identifiers and determine size
  // Since AudioGraph is a linked list, we traverse it.
  // Note: head is dummy.
  size_t maxId = 0;
  bool empty = true;
  AudioGraph::Node* current = audioGraph.head->next;

  while (current) {
    empty = false;
    if (current->test_node_identifier__ >= maxId) {
      maxId = current->test_node_identifier__;
    }
    current = current->next;
  }

  if (empty) return {};

  adjacencyList.resize(maxId + 1);

  current = audioGraph.head->next;
  while (current) {
    size_t nodeId = current->test_node_identifier__;
    for (AudioGraph::Node* input : current->inputs) {
      if (input) {
          adjacencyList[nodeId].push_back(input->test_node_identifier__);
      }
    }
    // Sort for consistent comparison
    std::sort(adjacencyList[nodeId].begin(), adjacencyList[nodeId].end());
    current = current->next;
  }

  return adjacencyList;
}

std::vector<std::vector<size_t>> TestGraphUtils::convertHostGraphToAdjacencyList(const HostGraph &hostGraph) {
  std::vector<std::vector<size_t>> adjacencyList;
  if (hostGraph.nodes.empty()) return {};

  size_t maxId = 0;
  for (const auto& node : hostGraph.nodes) {
    if (node->test_node_identifier__ > maxId) {
      maxId = node->test_node_identifier__;
    }
  }

  adjacencyList.resize(maxId + 1);

  for (const auto& node : hostGraph.nodes) {
    size_t nodeId = node->test_node_identifier__;
    for (HostGraph::Node* input : node->inputs) {
      if (input) {
        adjacencyList[nodeId].push_back(input->test_node_identifier__);
      }
    }
    std::sort(adjacencyList[nodeId].begin(), adjacencyList[nodeId].end());
  }

  return adjacencyList;
}

HostGraph TestGraphUtils::makeFromAdjacencyList(const std::vector<std::vector<size_t>> &adjacencyList) {
  HostGraph graph;
  // Create nodes
  for (size_t i = 0; i < adjacencyList.size(); ++i) {
    graph.nodes.emplace_back(std::make_unique<HostGraph::Node>());
    graph.nodes.back()->audioNode = new AudioGraph::Node(); // Create corresponding AudioGraph node
    graph.nodes.back()->test_node_identifier__ = i; // Set test identifier
    graph.nodes.back()->audioNode->test_node_identifier__ = i; // Set test identifier
  }

  // Create edges based on adjacency list
  for (size_t toIndex = 0; toIndex < adjacencyList.size(); ++toIndex) {
    for (size_t fromIndex : adjacencyList[toIndex]) {
      if (toIndex < graph.nodes.size() && fromIndex < graph.nodes.size()) {
          HostGraph::Node* fromNode = graph.nodes[fromIndex].get();
          HostGraph::Node* toNode = graph.nodes[toIndex].get();
          fromNode->outputs.push_back(toNode);
          toNode->inputs.push_back(fromNode);
          // Update AudioGraph nodes
          if (toNode->audioNode && fromNode->audioNode) {
              toNode->audioNode->inputs.push_back(fromNode->audioNode);
          }
      }
    }
  }

  size_t term = 1; // for traversal state management

  // This will be naive topological sort but this method is only intended for testing purposes so simplicity is more important than performance here
  std::sort(graph.nodes.begin(), graph.nodes.end(), [&term](const std::unique_ptr<HostGraph::Node>& a, const std::unique_ptr<HostGraph::Node>& b) {
    // we should swap if we can reach b from a
    std::vector<HostGraph::Node*> stack = {a.get()};

    term++;

    while (!stack.empty()) {
      HostGraph::Node* current = stack.back();
      stack.pop_back();
      if (current == b.get()) {
        return true;
      }
      if (current->traversalState.visit(term)) {
        for (HostGraph::Node* output : current->outputs) {
          stack.push_back(output);
        }
      }
    }
    return false; // a should not come before b
  });

  graph.last_term = term;

  if (!graph.nodes.empty()) {
      graph.head->next = graph.nodes[0].get();
      graph.nodes[0]->prev = graph.head;
      for (size_t i = 1; i < graph.nodes.size(); ++i) {
        graph.nodes[i-1]->next = graph.nodes[i].get();
        graph.nodes[i]->prev = graph.nodes[i-1].get();
        graph.nodes[i]->topologicalIndex = i;
      }
      graph.nodes.back()->next = graph.tail;
      graph.tail->prev = graph.nodes.back().get();
  } else {
      graph.head->next = graph.tail;
      graph.tail->prev = graph.head;
  }

  return graph;
}

AudioGraph TestGraphUtils::createAudioGraphFromHostGraph(const HostGraph &hostGraph) {
  AudioGraph audioGraph;
  HostGraph::Node *current = hostGraph.head->next;

  if (hostGraph.head->next != hostGraph.tail) {
       audioGraph.head->next = hostGraph.head->next->audioNode;
  } else {
       audioGraph.head->next = nullptr;
  }

  while (current != hostGraph.tail) {
    if (current->audioNode) {
        // Reconstruct inputs for AudioGraph::Node
        current->audioNode->inputs = std::vector<AudioGraph::Node*>(current->inputs.size());
        for (size_t i = 0; i < current->inputs.size(); ++i) {
          current->audioNode->inputs[i] = current->inputs[i]->audioNode;
        }

        current->audioNode->next = (current->next != hostGraph.tail) ? current->next->audioNode : nullptr;
    }
    current = current->next;
  }

  return audioGraph;
}

} // namespace audioapi::utils::graph

