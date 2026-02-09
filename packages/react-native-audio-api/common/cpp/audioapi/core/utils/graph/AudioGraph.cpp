#include <audioapi/core/utils/graph/AudioGraph.h>
#include <utility>

namespace audioapi::utils::graph {

AudioGraph::AudioGraph() {
  head = new Node(); // Create a dummy head node
}

AudioGraph::AudioGraph(AudioGraph&& other) noexcept : head(std::exchange(other.head, nullptr)) {}

AudioGraph& AudioGraph::operator=(AudioGraph&& other) noexcept {
  if (this != &other) {
    Node* current = head;
    while (current) {
      Node* nextNode = current->next;
      delete current;
      current = nextNode;
    }
    head = std::exchange(other.head, nullptr);
  }
  return *this;
}

AudioGraph::~AudioGraph() {
  Node* current = head;
  while (current) {
    Node* nextNode = current->next;
    delete current;
    current = nextNode;
  }
}

AudioGraph::Node* AudioGraph::Iterator::next() {
  if (!current) return nullptr;
  Node* result = current;
  current = current->next;
  return result;
}

} // namespace audioapi::utils::graph
