#include <audioapi/core/utils/graph/AudioGraph.h>
#include <utility>
#include <algorithm>

namespace audioapi::utils::graph {

AudioGraph::AudioGraph() {
    first_free_slot = 0;
}

AudioGraph::AudioGraph(AudioGraph&& other) noexcept
    : nodes(std::move(other.nodes)),
      executionOrder(std::move(other.executionOrder)),
      first_free_slot(other.first_free_slot),
      isDirty(other.isDirty) {
  other.isDirty = false;
  other.first_free_slot = 0;
}

AudioGraph& AudioGraph::operator=(AudioGraph&& other) noexcept {
  if (this != &other) {
    nodes = std::move(other.nodes);
    executionOrder = std::move(other.executionOrder);
    isDirty = other.isDirty;
    first_free_slot = other.first_free_slot;
    other.isDirty = false;
    other.first_free_slot = 0;
  }
  return *this;
}

uint32_t AudioGraph::createNode() {
  // Check if we have a free slot
  // If first_free_slot is within bounds, it's a hole.
  // If first_free_slot == size, we are at end.
  if (first_free_slot < static_cast<int32_t>(nodes.size())) {
      uint32_t idx = static_cast<uint32_t>(first_free_slot);

      // Move head to next
      first_free_slot = nodes[idx].next_free_slot;

      // Mark as taken
      nodes[idx].next_free_slot = -1;
      // Reset state
      nodes[idx].inputs.clear();
      nodes[idx].topo_out_degree = 0;

      return idx;
  }

  // No free slot, append
  nodes.emplace_back();
  uint32_t idx = static_cast<uint32_t>(nodes.size() - 1);
  nodes[idx].next_free_slot = -1; // Active

  // Maintain first_free_slot pointing to the "next potential" (which is now new size)
  first_free_slot = static_cast<int32_t>(nodes.size());

  return idx;
}

void AudioGraph::releaseNode(uint32_t index) {
  if (index >= nodes.size()) return;

  // Check if already free
  if (!nodes[index].isActive()) return;

  // Clear data
  nodes[index].inputs.clear();

  // Add to head of free list
  nodes[index].next_free_slot = first_free_slot;
  first_free_slot = static_cast<int32_t>(index);
}

void AudioGraph::process() {
  if (isDirty) {
    recomputeTopologicalOrder();
    isDirty = false;
  }

  // Actually execute. For now this is just structure maintenance.
  // In a real audio graph, we would do:
  // for (uint32_t idx : executionOrder) {
  //    nodes[idx].process();
  // }
}

void AudioGraph::recomputeTopologicalOrder() {
  executionOrder.clear();
  if (nodes.empty()) return;
  executionOrder.reserve(nodes.size());

  // Khan's Algorithm (Reverse Mode on Reversed Graph)
  // Our graph stores Inputs (Back-Edges).
  // Node U stores [V1, V2] meaning V1->U and V2->U.

  // Reverse Kahn: Needs Out-Degree and Backward Edges (V -> U).

  // 1. Compute Out-Degree (Number of Dependents) for each node.
  // NOTE: here out degree will always be 0 so we do not need to init it to 0

  // Iterate all nodes to count usage.
  for (const auto& node : nodes) {
    if (!node.isActive()) continue;

    for (uint32_t inputIdx : node.inputs) {
      if (inputIdx < nodes.size() && nodes[inputIdx].isActive()) {
        nodes[inputIdx].topo_out_degree++;
      }
    }
  }

  // 2. Queue of 0-Out-Degree Nodes (Leaves).
  // These are nodes that nothing depends on (or final destinations).
  // We collect them into `executionOrder` temporarily acting as queue.
  size_t queueStart = 0;

  for (size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].isActive() && nodes[i].topo_out_degree == 0) {
      executionOrder.push_back(static_cast<uint32_t>(i));
    }
  }

  // 3. Process Queue
  while (queueStart < executionOrder.size()) {
    uint32_t vIdx = executionOrder[queueStart++];

    // For each node U that is an input of V (U -> V)
    // Since we process leaves first (V is a leaf in the remaining sub-graph),
    // V is 'satisfied' or 'removed'. We reduce the Out-Degree of U.

    const Node& v = nodes[vIdx];
    for (uint32_t uIdx : v.inputs) {
      if (uIdx < nodes.size()) {
        Node& u = nodes[uIdx];
        if (u.isActive()) {
          if (u.topo_out_degree > 0) {
            u.topo_out_degree--;
            if (u.topo_out_degree == 0) {
              executionOrder.push_back(uIdx);
            }
          }
        }
      }
    }
  }

  // We now have a Reverse Topological Order in `executionOrder`.
  // Leaf (Destination) -> Source.
  // We want Source -> Destination for execution (Dependency -> Dependent).
  std::reverse(executionOrder.begin(), executionOrder.end());
}

} // namespace audioapi::utils::graph
