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


std::pair<HostGraph::Node*, HostGraph::AGEvent> HostGraph::addNode(std::shared_ptr<NodeHandle> handle) {
  collectDisposedNodes();

  Node* newNode = new Node();
  newNode->handle = handle;
  nodes.push_back(newNode);

  auto event = [h = std::move(handle)](AudioGraph& graph) {
    graph.addNode(h);
  };

  return {newNode, std::move(event)};
}

HostGraph::Res HostGraph::removeNode(Node *node) {
  auto it = std::find(nodes.begin(), nodes.end(), node);
  if (it == nodes.end()) {
      return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  // Mark as ghost — keep edges intact so hasPath can still detect paths through it.
  // The ghost will be cleaned up by collectDisposedNodes() once AudioGraph
  // releases its shared_ptr (use_count drops to 1).
  node->ghost = true;

  return Res::Ok([h = node->handle](AudioGraph& graph) {
      graph[h->index].orphaned = true;
  });
}

HostGraph::Res HostGraph::addEdge(Node *from, Node *to) {
  collectDisposedNodes();

  // Check if nodes exist in graph and are not ghosts
  if (std::find(nodes.begin(), nodes.end(), from) == nodes.end() ||
      std::find(nodes.begin(), nodes.end(), to) == nodes.end()) {
      return Res::Err(ResultError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
      return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  // Check if edge exists
  for (Node* out : from->outputs) {
      if (out == to) return Res::Err(ResultError::EDGE_ALREADY_EXISTS);
  }

  // Check for cycle: look for path from 'to' to 'from'
  // hasPath traverses all nodes including ghosts, so cycle detection
  // correctly accounts for nodes still alive in AudioGraph.
  if (hasPath(to, from)) {
      return Res::Err(ResultError::CYCLE_DETECTED);
  }

  from->outputs.push_back(to);
  to->inputs.push_back(from);

  return Res::Ok([hFrom = from->handle, hTo = to->handle](AudioGraph& graph) {
      graph[hTo->index].inputs.push_back(hFrom->index);
      graph.markDirty();
  });
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

HostGraph::Res HostGraph::removeEdge(Node *from, Node *to) {
  collectDisposedNodes();

  // Check existence (reject ghosts)
  if (std::find(nodes.begin(), nodes.end(), from) == nodes.end() ||
      std::find(nodes.begin(), nodes.end(), to) == nodes.end()) {
      return Res::Err(ResultError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
      return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  auto itOut = std::find(from->outputs.begin(), from->outputs.end(), to);
  if (itOut == from->outputs.end()) return Res::Err(ResultError::EDGE_NOT_FOUND);

  auto itIn = std::find(to->inputs.begin(), to->inputs.end(), from);
  if (itIn != to->inputs.end()) {
      to->inputs.erase(itIn);
  }
  from->outputs.erase(itOut);

  return Res::Ok([hFrom = from->handle, hTo = to->handle](AudioGraph& graph) {
      auto& inputs = graph[hTo->index].inputs;
      auto itIn = std::remove(inputs.begin(), inputs.end(), hFrom->index);
      if (itIn != inputs.end()) inputs.erase(itIn, inputs.end());

      graph.markDirty();
  });
}

void HostGraph::collectDisposedNodes() {
  for (auto it = nodes.begin(); it != nodes.end(); ) {
    Node* n = *it;
    // A ghost whose handle has use_count == 1 means AudioGraph released its ref.
    // Safe to destroy the ghost and the AudioNode on the main thread.
    if (n->ghost && n->handle.use_count() == 1) {
      *it = nodes.back();
      nodes.pop_back();
      delete n; // ~Node tears down edges; last shared_ptr destroys NodeHandle + AudioNode
    } else {
      ++it;
    }
  }
}

}; // namespace audioapi::utils::graph

