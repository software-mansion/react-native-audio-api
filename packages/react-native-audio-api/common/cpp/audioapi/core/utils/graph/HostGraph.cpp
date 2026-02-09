#include <audioapi/core/utils/graph/HostGraph.h>
#include <utility>

namespace audioapi::utils::graph {

bool HostGraph::TraversalState::visit(size_t currentTerm) {
  const bool ret = (term != currentTerm);
  term = currentTerm;
  return ret;
}

HostGraph::Node::~Node() {
  // Remove this node from its inputs' outputs
  for (Node* input : inputs) {
    input->outputs.erase(std::remove(input->outputs.begin(), input->outputs.end(), this), input->outputs.end());
  }
  // Remove this node from its outputs' inputs
  for (Node* output : outputs) {
    output->inputs.erase(std::remove(output->inputs.begin(), output->inputs.end(), this), output->inputs.end());
  }
  if (prev != nullptr) {
    prev->next = next;
  }
  if (next != nullptr) {
    next->prev = prev;
  }
}

HostGraph::HostGraph() {
  head = new Node(); // Create a dummy head node
  tail = new Node(); // Create a dummy tail node
  head->next = tail;
  tail->prev = head;
}

HostGraph::HostGraph(HostGraph&& other) noexcept
    : nodes(std::move(other.nodes)),
      head(std::exchange(other.head, nullptr)),
      tail(std::exchange(other.tail, nullptr)),
      last_term(other.last_term) {
  other.last_term = 0;
}

HostGraph& HostGraph::operator=(HostGraph&& other) noexcept {
  if (this != &other) {
    // Clean up existing resources before overwriting

    // Optimization: clear edges to speed up destruction
    for (const auto& nodePtr : nodes) {
        if(nodePtr) {
            nodePtr->inputs.clear();
            nodePtr->outputs.clear();
        }
    }
    nodes.clear();
    delete head;
    delete tail;

    // Move resources
    nodes = std::move(other.nodes);

    head = std::exchange(other.head, nullptr);
    tail = std::exchange(other.tail, nullptr);
    last_term = other.last_term;
    other.last_term = 0;
  }
  return *this;
}

HostGraph::~HostGraph() {
  // For faster cleanup we will empty out all edges to avoid O(n^2) complexity
  for (const auto& nodePtr : nodes) {
    if (nodePtr) {
      nodePtr->inputs.clear();
      nodePtr->outputs.clear();
    }
  }
  // Nodes will be automatically deleted by unique_ptr destructors

  delete head;
  delete tail;
}

std::pair<HostGraph::Node*, HostGraph::AGEvent> HostGraph::addNode(AudioGraph::Node *audioNode) {
  return {}; // TODO
}

HostGraph::AGEvent HostGraph::removeNode(Node *node) {
  return {}; // TODO
}

HostGraph::AGEvent HostGraph::addEdge(Node *from, Node *to) {
  return {}; // TODO
}

HostGraph::AGEvent HostGraph::removeEdge(Node *from, Node *to) {
  return {}; // TODO
}

}; // namespace audioapi::utils::graph
