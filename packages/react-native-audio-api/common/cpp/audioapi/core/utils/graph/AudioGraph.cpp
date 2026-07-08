#include <audioapi/core/utils/graph/AudioGraph.h>
#include <utility>

namespace audioapi::utils::graph {

// ── Accessors ─────────────────────────────────────────────────────────────

auto AudioGraph::operator[](std::uint32_t index) -> Node & {
  return nodes[index];
}

auto AudioGraph::operator[](std::uint32_t index) const -> const Node & {
  return nodes[index];
}

size_t AudioGraph::size() const {
  return nodes.size();
}

bool AudioGraph::empty() const {
  return nodes.empty();
}

InputPool &AudioGraph::pool() {
  return pool_;
}

const InputPool &AudioGraph::pool() const {
  return pool_;
}

// ── Mutators & processing ─────────────────────────────────────────────────

void AudioGraph::reserveNodes(std::uint32_t capacity) {
  nodes.reserve(capacity);
}

void AudioGraph::markDirty() {
  topo_order_dirty = true;
}

void AudioGraph::addNode(std::shared_ptr<NodeHandle> handle) {
  handle->index = static_cast<std::uint32_t>(nodes.size());
  nodes.emplace_back(std::move(handle));
}

void AudioGraph::process() {
  if (topo_order_dirty) {
    topo_order_dirty = false;
    kahn_toposort();
    if (topo_order_dirty) {
      return;
    }
  }

  const auto n = static_cast<std::uint32_t>(nodes.size());

  // ── Pass 1: mark deletions (cascading, left-to-right in topo order) ────
  // A node is deleted when: orphaned && no live inputs && canBeDestructed().
  // Because the array is topologically sorted, removing a source first lets
  // its dependents see the updated input set and potentially cascade.
  for (auto &node : nodes) {
    pool_.removeIf(
        node.input_head, [this](std::uint32_t inp) { return nodes[inp].will_be_deleted; });

    if (node.orphaned && InputPool::isEmpty(node.input_head) &&
        node.handle->audioNode->canBeDestructed()) {
      node.will_be_deleted = true;
    }
  }

  // ── Compute new-position remap (stored in after_compaction_ind) ─────────
  std::uint32_t new_pos = 0;
  for (std::uint32_t i = 0; i < n; i++) {
    if (!nodes[i].will_be_deleted) {
      nodes[i].after_compaction_ind = static_cast<std::int32_t>(new_pos);
      new_pos++;
    }
    // deleted nodes keep after_compaction_ind == -1 (default)
  }

  // ── Pass 2a: remap inputs to post-compaction indices ─────────────────────
  // Must happen BEFORE shifting nodes, because shifting invalidates source
  // positions that later nodes' inputs may still reference.
  for (std::uint32_t e = 0; e < n; e++) {
    if (nodes[e].will_be_deleted) {
      continue;
    }
    for (auto &inp : pool_.mutableView(nodes[e].input_head)) {
      inp = static_cast<std::uint32_t>(nodes[inp].after_compaction_ind);
    }
  }

  // ── Pass 2b: compact — shift kept nodes left ───────────────────────────
  std::uint32_t b = 0;
  for (std::uint32_t e = 0; e < n; e++) {
    if (nodes[e].will_be_deleted) {
      continue;
    }
    if (b != e) {
      nodes[b] = std::move(nodes[e]);
      nodes[e].input_head = InputPool::kNull; // prevent double-free in truncation
    }
    nodes[b].handle->index = b;
    b++;
  }

  // Truncate — dropping shared_ptr decrements refcount (2 → 1);
  // HostGraph detects this and destroys the ghost on the main thread.
  for (std::uint32_t i = b; i < n; i++) {
    // Free any lingering pool slots (should already be empty for deleted nodes)
    pool_.freeAll(nodes[i].input_head);
    // Handle may have been moved-from during compaction, so just null it
    nodes[i].handle = nullptr;
  }
  nodes.resize(b);

  // Reset scratch fields for next compaction
  for (auto &node : nodes) {
    node.after_compaction_ind = -1;
    node.will_be_deleted = false;
  }
}

// ── Kahn's toposort ───────────────────────────────────────────────────────

void AudioGraph::kahn_toposort() {
  const auto n = static_cast<std::uint32_t>(nodes.size());
  if (n <= 1) {
    return;
  }

  // Phase 1: compute out-degree
  for (const auto &nd : nodes) {
    for (std::uint32_t inp : pool_.view(nd.input_head)) {
      nodes[inp].topo_out_degree++;
    }
  }

  // Phase 2: reverse Kahn BFS — sinks first, sources last in dequeue order.
  // FIFO queue embedded as a linked list through after_compaction_ind.
  std::int32_t qh = -1, qt = -1;
  auto enq = [&](std::uint32_t i) {
    nodes[i].after_compaction_ind = -1;
    if (qh == -1) [[unlikely]] {
      qh = qt = static_cast<std::int32_t>(i);
    } else {
      nodes[qt].after_compaction_ind = static_cast<std::int32_t>(i);
      qt = static_cast<std::int32_t>(i);
    }
  };

  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].topo_out_degree == 0) {
      enq(i);
    }
  }

  std::uint32_t write = n;
  while (qh != -1) {
    auto idx = static_cast<std::uint32_t>(qh);
    qh = nodes[idx].after_compaction_ind;
    nodes[idx].after_compaction_ind = static_cast<std::int32_t>(--write);

    for (std::uint32_t inp : pool_.view(nodes[idx].input_head)) {
      if (--nodes[inp].topo_out_degree == 0) {
        enq(inp);
      }
    }
  }

  // Phase 3: remap input indices to new positions (before nodes move)
  for (auto &nd : nodes) {
    for (std::uint32_t &inp : pool_.mutableView(nd.input_head)) {
      inp = static_cast<std::uint32_t>(nodes[inp].after_compaction_ind);
    }
  }

  // Phase 4: apply permutation in place via cycle sort
  for (std::uint32_t i = 0; i < n; i++) {
    while (nodes[i].after_compaction_ind != static_cast<std::int32_t>(i)) {
      auto t = static_cast<std::uint32_t>(nodes[i].after_compaction_ind);
      std::swap(nodes[i], nodes[t]);
    }
  }

  // Phase 5: update handle indices & reset scratch
  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].handle) {
      nodes[i].handle->index = i;
    }
    nodes[i].after_compaction_ind = -1;
  }
}

AudioGraph::NodeBuffer AudioGraph::adoptNodeBuffer(NodeBuffer preAllocated) {
  // Move live nodes into the pre-allocated (empty, large-capacity) buffer.
  // No reallocation: preAllocated.data.capacity() >= nodes.size() guaranteed
  // by the main thread before sending this event.
  preAllocated.data.insert(
      preAllocated.data.end(),
      std::make_move_iterator(nodes.begin()),
      std::make_move_iterator(nodes.end()));
  std::swap(nodes, preAllocated.data);
  // preAllocated.data now holds the old (small) buffer with moved-from nodes.
  // Caller disposes it off the audio thread.
  return preAllocated;
}

} // namespace audioapi::utils::graph
