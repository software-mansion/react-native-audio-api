#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <algorithm>
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

void AudioGraph::markDeletions() {
  auto flagged = [this](std::uint32_t idx) {
    return nodes[idx].will_be_deleted;
  };

  // Decide first. A node goes when it is orphaned, every input is going too,
  // and it agrees.
  for (auto &node : nodes) {
    node.will_be_deleted = node.orphaned &&
        std::ranges::all_of(pool_.view(node.input_head), flagged) &&
        node.handle->audioNode->canBeDestructed();
  }

  // Then scrub. Every flag is final now.
  for (auto &node : nodes) {
    if (node.will_be_deleted) {
      continue;
    }
    forEachDependencyList(node, [&](std::uint32_t &head) { pool_.removeIf(head, flagged); });
  }
}

void AudioGraph::remapListsToTargetIndex() {
  // Must run BEFORE nodes move: once they do, an index stored in a list no
  // longer names the node it was written for.
  for (auto &node : nodes) {
    forEachDependencyList(node, [this](std::uint32_t head) {
      for (auto &dep : pool_.mutableView(head)) {
        dep = static_cast<std::uint32_t>(nodes[dep].target_index);
      }
    });
  }
}

void AudioGraph::sortAndCompact() {
  if (topo_order_dirty) {
    topo_order_dirty = false;
    kahn_toposort();
  }

  // Only orphaned nodes can be deleted, so with none present the passes below
  // would walk every input list and change nothing.
  if (std::ranges::none_of(nodes, [](const Node &node) { return node.orphaned; })) {
    return;
  }

  const auto n = static_cast<std::uint32_t>(nodes.size());

  markDeletions();

  // ── Assign each survivor its post-compaction position ───────────────────
  // Deleted nodes keep target_index == -1 and give their pool slots back now,
  // so the remap below never has to special-case them.
  std::uint32_t new_pos = 0;
  for (auto &node : nodes) {
    if (node.will_be_deleted) {
      forEachDependencyList(node, [this](std::uint32_t &head) { pool_.freeAll(head); });
    } else {
      node.target_index = static_cast<std::int32_t>(new_pos++);
    }
  }

  remapListsToTargetIndex();

  // ── Pass 2b: compact — shift kept nodes left ───────────────────────────
  std::uint32_t b = 0;
  for (std::uint32_t e = 0; e < n; e++) {
    if (nodes[e].will_be_deleted) {
      continue;
    }
    if (b != e) {
      nodes[b] = std::move(nodes[e]);
    }
    nodes[b].handle->index = b;
    b++;
  }

  // Truncate — dropping shared_ptr decrements refcount (2 → 1);
  // HostGraph detects this and destroys the ghost on the main thread.
  // Handles may have been moved-from during compaction, so just null them.
  for (std::uint32_t i = b; i < n; i++) {
    nodes[i].handle = nullptr;
  }
  nodes.resize(b);

  // Reset scratch fields for next compaction
  for (auto &node : nodes) {
    node.target_index = -1;
    node.will_be_deleted = false;
  }
}

void AudioGraph::settleProcessableState() {
  using PS = GraphObject::PROCESSABLE_STATE;

  std::int32_t top = -1;
  auto push = [&](std::uint32_t i) {
    nodes[i].target_index = top;
    top = static_cast<std::int32_t>(i);
  };

  for (std::uint32_t i = 0; i < nodes.size(); i++) {
    if (nodes[i].handle->audioNode->processableState_ != PS::NOT_PROCESSABLE) {
      push(i);
    }
  }

  // Promote each popped node's dependencies to CONDITIONAL_PROCESSABLE and
  // push the ones that transitioned. A node that opted out via
  // excludeFromProcessablePull_ stays NOT_PROCESSABLE and is never pushed, so
  // nothing propagates through it.
  while (top != -1) {
    const auto idx = static_cast<std::uint32_t>(top);
    top = nodes[idx].target_index;
    nodes[idx].target_index = -1;

    forEachDependencyList(nodes[idx], [&](std::uint32_t head) {
      for (auto dep : pool_.view(head)) {
        auto &obj = *nodes[dep].handle->audioNode;
        if (obj.processableState_ == PS::NOT_PROCESSABLE && !obj.excludeFromProcessablePull_) {
          obj.processableState_ = PS::CONDITIONAL_PROCESSABLE;
          push(dep);
        }
      }
    });
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
    for (auto inp : pool_.view(nd.input_head)) {
      nodes[inp].topo_out_degree++;
    }
  }

  // Phase 2: reverse Kahn — sinks first, sources last in pop order.
  std::int32_t top = -1;
  auto push = [&](std::uint32_t i) {
    nodes[i].target_index = top; // temporary: link to the node below on the ready stack
    top = static_cast<std::int32_t>(i);
  };

  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].topo_out_degree == 0) {
      push(i);
    }
  }

  std::uint32_t write = n;
  while (top != -1) {
    const auto idx = static_cast<std::uint32_t>(top);
    top = nodes[idx].target_index;
    nodes[idx].target_index = static_cast<std::int32_t>(--write); // final: position after the sort

    for (auto inp : pool_.view(nodes[idx].input_head)) {
      if (--nodes[inp].topo_out_degree == 0) {
        push(inp);
      }
    }
  }

  // Phase 3: remap input (and link) indices to new positions (before nodes move)
  remapListsToTargetIndex();

  // Phase 4: apply permutation in place via cycle sort
  for (std::uint32_t i = 0; i < n; i++) {
    while (nodes[i].target_index != static_cast<std::int32_t>(i)) {
      const auto t = static_cast<std::uint32_t>(nodes[i].target_index);
      std::swap(nodes[i], nodes[t]);
    }
  }

  // Phase 5: update handle indices & reset scratch
  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].handle) {
      nodes[i].handle->index = i;
    }
    nodes[i].target_index = -1;
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
