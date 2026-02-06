#include <audioapi/core/utils/graph/AudioGraph.h>
#include <utility>

namespace audioapi::utils::graph {

void AudioGraph::swapNodesInTopologicalOrder(Node* nodeA, Node* nodeB) {
  if (nodeA == nodeB) return;

  bool adjacent = (nodeA->next == nodeB) || (nodeB->next == nodeA);

  if (adjacent) {
    Node* first = (nodeA->next == nodeB) ? nodeA : nodeB;
    Node* second = (first == nodeA) ? nodeB : nodeA;

    Node* prevFirst = first->prev;
    Node* nextSecond = second->next;

    if (prevFirst) prevFirst->next = second;
    if (nextSecond) nextSecond->prev = first;

    second->prev = prevFirst;
    second->next = first;
    first->prev = second;
    first->next = nextSecond;

    if (head == first) head = second;
  } else {
    Node* prevA = nodeA->prev;
    Node* nextA = nodeA->next;
    Node* prevB = nodeB->prev;
    Node* nextB = nodeB->next;

    if (prevA) prevA->next = nodeB;
    if (nextA) nextA->prev = nodeB;

    if (prevB) prevB->next = nodeA;
    if (nextB) nextB->prev = nodeA;

    nodeA->prev = prevB;
    nodeA->next = nextB;
    nodeB->prev = prevA;
    nodeB->next = nextA;

    if (head == nodeA) head = nodeB;
    else if (head == nodeB) head = nodeA;
  }

  std::swap(nodeA->topologicalIndex, nodeB->topologicalIndex);
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
