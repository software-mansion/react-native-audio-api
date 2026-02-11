#include <audioapi/core/utils/graph/HostGraph.h>
#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

bool HostGraph::TraversalState::visit(size_t currentTerm) {
    if (term == currentTerm) return false;
    term = currentTerm;
    return true;
}

HostGraph::Node::~Node() {
  // Remove this node from its inputs' outputs
  for (Node* input : inputs) {
    auto& outs = input->outputs;
    outs.erase(std::remove(outs.begin(), outs.end(), this), outs.end());
  }
  // Remove this node from its outputs' inputs
  for (Node* output : outputs) {
    auto& inps = output->inputs;
    inps.erase(std::remove(inps.begin(), inps.end(), this), inps.end());
  }
}

HostGraph::HostGraph() {
}

HostGraph::HostGraph(HostGraph&& other) noexcept
    : nodes(std::move(other.nodes)),
      last_term(other.last_term) {
  other.last_term = 0;
}

HostGraph& HostGraph::operator=(HostGraph&& other) noexcept {
  if (this != &other) {
     for (Node* n : nodes) delete n;
     nodes = std::move(other.nodes);
     last_term = other.last_term;
     other.last_term = 0;
  }
  return *this;
}

HostGraph::~HostGraph() {
  for (Node* n : nodes) delete n;
  nodes.clear();
}


std::pair<HostGraph::Node*, HostGraph::AGEvent> HostGraph::addNode(uint32_t audioNodeIndex) {
  Node* newNode = new Node();
  newNode->audioNodeIndex = audioNodeIndex;
  nodes.push_back(newNode);

  auto event = [audioNodeIndex](AudioGraph& graph, Disposer&) {
    // Ensure the node exists in the AudioGraph if it doesn't already
    // This assumes sequential addition logic or that caller handled it.
    // If we want to be safe:
    if (graph.nodes.size() <= audioNodeIndex) {
        graph.nodes.resize(audioNodeIndex + 1);
    }
  };

  return {newNode, event};
}

HostGraph::AGEvent HostGraph::removeNode(Node *node) {
  auto it = std::find(nodes.begin(), nodes.end(), node);
  if (it != nodes.end()) {
      *it = nodes.back();
      nodes.pop_back();
  }

  uint32_t targetIdx = node->audioNodeIndex;
  delete node;

  return [targetIdx](AudioGraph& graph, Disposer&) {
      // "Ghost Node" strategy: Clear inputs so it disconnects from graph logic
      if (targetIdx < graph.nodes.size()) {
          graph.nodes[targetIdx].inputs.clear();
      }

      for (auto& n : graph.nodes) {
          if (!n.isActive()) continue;

          auto& inps = n.inputs;
          auto removeIt = std::remove(inps.begin(), inps.end(), targetIdx);
          if (removeIt != inps.end()) {
              inps.erase(removeIt, inps.end());
          }
      }

      graph.markDirty();
  };
}

HostGraph::AGEvent HostGraph::addEdge(Node *from, Node *to) {
  // Check if edge exists
  for (Node* out : from->outputs) {
      if (out == to) return [](AudioGraph&, Disposer&){};
  }

  // Check for cycle: look for path from 'to' to 'from'
  if (hasPath(to, from)) {
      return [](AudioGraph&, Disposer&){};
  }

  from->outputs.push_back(to);
  to->inputs.push_back(from);

  return [fromIdx = from->audioNodeIndex, toIdx = to->audioNodeIndex](AudioGraph& graph, Disposer&) {
      if (toIdx < graph.nodes.size() && fromIdx < graph.nodes.size()) {
        graph.nodes[toIdx].inputs.push_back(fromIdx);
        graph.markDirty();
      }
  };
}

bool HostGraph::hasPath(Node* start, Node* end) {
    if (start == end) return true;

    last_term++;
    size_t term = last_term;

    std::vector<Node*> stack;
    stack.push_back(start);
    start->traversalState.term = term;

    while (!stack.empty()) {
        Node* curr = stack.back();
        stack.pop_back();

        if (curr == end) return true;

        for (Node* out : curr->outputs) {
            if (out->traversalState.visit(term)) {
                stack.push_back(out);
            }
        }
    }
    return false;
}

HostGraph::AGEvent HostGraph::removeEdge(Node *from, Node *to) {
  // Check existence
  auto itOut = std::find(from->outputs.begin(), from->outputs.end(), to);
  if (itOut == from->outputs.end()) return [](AudioGraph&, Disposer&){};

  auto itIn = std::find(to->inputs.begin(), to->inputs.end(), from);
  if (itIn != to->inputs.end()) {
      to->inputs.erase(itIn);
  }
  from->outputs.erase(itOut);

  return [fromIdx = from->audioNodeIndex, toIdx = to->audioNodeIndex](AudioGraph& graph, Disposer&) {
        if (toIdx < graph.nodes.size() && fromIdx < graph.nodes.size()) {
            auto& inputs = graph.nodes[toIdx].inputs;
            auto itIn = std::remove(inputs.begin(), inputs.end(), fromIdx);
            if (itIn != inputs.end()) inputs.erase(itIn, inputs.end());

            graph.markDirty();
        }
  };
}

}; // namespace audioapi::utils::graph

