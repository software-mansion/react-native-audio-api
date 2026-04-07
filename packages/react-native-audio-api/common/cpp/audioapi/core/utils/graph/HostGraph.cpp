#include <audioapi/core/AudioContext.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/core/utils/graph/GraphObject.h>
#include <audioapi/core/utils/graph/HostGraph.h>

#include <algorithm>
#include <cstddef>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

// =========================================================================
// Implementation
// =========================================================================

bool HostGraph::TraversalState::visit(size_t currentTerm) {
  if (term == currentTerm) {
    return false;
  }
  term = currentTerm;
  return true;
}

HostGraph::Node::~Node() {
  for (Node *input : inputs) {
    auto &outs = input->outputs;
    outs.erase(std::remove(outs.begin(), outs.end(), this), outs.end());
  }
  for (Node *output : outputs) {
    auto &inps = output->inputs;
    inps.erase(std::remove(inps.begin(), inps.end(), this), inps.end());
  }
}

HostGraph::HostGraph() = default;

HostGraph::~HostGraph() {
  for (Node *n : nodes) {
    delete n;
  }
  nodes.clear();
}

HostGraph::HostGraph(HostGraph &&other) noexcept
    : nodes(std::move(other.nodes)), edgeCount_(other.edgeCount_), last_term(other.last_term) {
  other.edgeCount_ = 0;
  other.last_term = 0;
}

auto HostGraph::operator=(HostGraph &&other) noexcept -> HostGraph & {
  if (this != &other) {
    for (Node *n : nodes) {
      delete n;
    }
    nodes = std::move(other.nodes);
    edgeCount_ = other.edgeCount_;
    last_term = other.last_term;
    other.edgeCount_ = 0;
    other.last_term = 0;
  }
  return *this;
}

auto HostGraph::addNode(std::shared_ptr<NodeHandle> handle) -> std::pair<Node *, AGEvent> {
  Node *newNode = new Node();
  newNode->handle = handle;
  nodes.push_back(newNode);

  auto event = [h = std::move(handle)](auto &graph, auto &) {
    graph.addNode(h);
  };

  return {newNode, std::move(event)};
}

auto HostGraph::removeNode(Node *node) -> Res {
  auto it = std::ranges::find(nodes, node);
  if (it == nodes.end()) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  node->ghost = true;

  return Res::Ok(
      [h = node->handle](AudioGraph &graph, auto &) { graph[h->index].orphaned = true; });
}

void HostGraph::markNodesAsProcessing(Node *node) {
  if (node == nullptr) {
    return;
  }
  if (!node->handle->audioNode->isProcessable()) {
    node->handle->audioNode->setProcessableState(
        GraphObject::PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE);
  }
  if (node->inputs.empty()) {
    return;
  }

  for (Node *input : node->inputs) {
    markNodesAsProcessing(input);
  }
}

auto HostGraph::addEdge(Node *from, Node *to) -> Res {
  if (std::ranges::find(nodes, from) == nodes.end() ||
      std::ranges::find(nodes, to) == nodes.end()) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  for (Node *out : from->outputs) {
    if (out == to) {
      return Res::Err(ResultError::EDGE_ALREADY_EXISTS);
    }
  }

  if (hasPath(to, from)) {
    return Res::Err(ResultError::CYCLE_DETECTED);
  }

  from->outputs.push_back(to);
  to->inputs.push_back(from);
  edgeCount_++;

  // could be problematic, since we are passing raw pointers to the lambda
  return Res::Ok([from, to](AudioGraph &graph, auto &) {
    if (!from->handle->audioNode->isProcessable() && to->handle->audioNode->isProcessable()) {
      markNodesAsProcessing(from);
    }
    graph.pool().push(graph[to->handle->index].input_head, from->handle->index);
    graph.markDirty();
  });
}

void HostGraph::markNodesAsNotProcessing(Node *node) {
  if (node == nullptr) {
    return;
  }
  if (!node->handle->audioNode->isProcessable()) {
    return;
  }
  if (node->handle->audioNode->processableState_ ==
      GraphObject::PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE) {
    node->handle->audioNode->setProcessableState(GraphObject::PROCESSABLE_STATE::NOT_PROCESSABLE);
  }
  if (node->inputs.empty()) {
    return;
  }

  for (Node *input : node->inputs) {
    markNodesAsNotProcessing(input);
  }
}

auto HostGraph::removeEdge(Node *from, Node *to) -> Res {
  if (std::ranges::find(nodes, from) == nodes.end() ||
      std::ranges::find(nodes, to) == nodes.end()) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  auto itOut = std::ranges::find(from->outputs, to);
  if (itOut == from->outputs.end()) {
    return Res::Err(ResultError::EDGE_NOT_FOUND);
  }

  auto itIn = std::ranges::find(to->inputs, from);
  if (itIn != to->inputs.end()) {
    to->inputs.erase(itIn);
  }
  from->outputs.erase(itOut);
  edgeCount_--;

  // could be problematic, since we are passing raw pointers to the lambda
  return Res::Ok([from, to](AudioGraph &graph, auto &) {
    if (from != nullptr &&
        from->handle->audioNode->processableState_ ==
            GraphObject::PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE) {
      bool updateProcessingNodes = std::ranges::all_of(
          from->outputs, [](Node *output) { return !output->handle->audioNode->isProcessable(); });
      if (updateProcessingNodes) {
        HostGraph::markNodesAsNotProcessing(from);
      }
    }
    graph.pool().remove(graph[to->handle->index].input_head, from->handle->index);
    graph.markDirty();
  });
}

auto HostGraph::removeAllEdges(Node *from) -> Res {
  if (std::ranges::find(nodes, from) == nodes.end() || from->ghost) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  auto pairs = std::vector<std::pair<std::uint32_t, std::uint32_t>>();
  pairs.reserve(from->outputs.size());

  for (Node *to : from->outputs) {
    auto itIn = std::ranges::find(to->inputs, from);
    if (itIn != to->inputs.end()) {
      to->inputs.erase(itIn);
    }
    edgeCount_--;
    pairs.emplace_back(from->handle->index, to->handle->index);
  }
  from->outputs.clear();

  return Res::Ok([pairs = std::move(pairs), from](AudioGraph &graph, auto &disposer) mutable {
    auto *fromNode = from->handle->audioNode->asAudioNode();
    if (fromNode != nullptr &&
        fromNode->processableState_ == GraphObject::PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE) {
      HostGraph::markNodesAsNotProcessing(from);
    }
    for (const auto &[fromIdx, toIdx] : pairs) {
      graph.pool().remove(graph[toIdx].input_head, fromIdx);
    }
    graph.markDirty();
    disposer.dispose(std::move(pairs));
  });
}

bool HostGraph::hasPath(Node *start, Node *end) {
  if (start == end) {
    return true;
  }

  last_term++;
  size_t term = last_term;

  std::vector<Node *> stack;
  stack.push_back(start);
  start->traversalState.term = term;

  while (!stack.empty()) {
    Node *curr = stack.back();
    stack.pop_back();

    if (curr == end) {
      return true;
    }

    for (Node *out : curr->outputs) {
      if (out->traversalState.visit(term)) {
        stack.push_back(out);
      }
    }
  }
  return false;
}

size_t HostGraph::edgeCount() const {
  return edgeCount_;
}

size_t HostGraph::nodeCount() const {
  return nodes.size();
}

void HostGraph::collectDisposedNodes() {
  for (auto it = nodes.begin(); it != nodes.end();) {
    Node *n = *it;
    if (n->ghost && n->handle.use_count() == 1) {
      edgeCount_ -= n->outputs.size();
      *it = nodes.back();
      nodes.pop_back();
      delete n;
    } else {
      ++it;
    }
  }
}

} // namespace audioapi::utils::graph
