#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/graph/AudioGraph.h>
#include <algorithm>
#include <cassert>
#include <iterator>
#include <utility>

namespace audioapi::utils::graph {

// ── Accessors ─────────────────────────────────────────────────────────────

auto AudioGraph::operator[](std::uint32_t index) -> Vertex & {
  return nodes[index];
}

auto AudioGraph::operator[](std::uint32_t index) const -> const Vertex & {
  return nodes[index];
}

auto AudioGraph::operator[](const NodeHandle &handle) -> Vertex & {
  return nodes[handle.index_];
}

auto AudioGraph::operator[](const NodeHandle &handle) const -> const Vertex & {
  return nodes[handle.index_];
}

std::uint32_t AudioGraph::indexOf(const NodeHandle &handle) const {
  return handle.index_;
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
  handle->index_ = static_cast<std::uint32_t>(nodes.size());
  nodes.emplace_back(std::move(handle));
}

void AudioGraph::markDeletions() {
  auto flagged = [this](std::uint32_t idx) {
    return nodes[idx].will_be_deleted;
  };

  // A node goes when it is orphaned, it agrees, and every input is going too.
  // The nodes of a cycle wait on each other, so assume every orphaned candidate will be deleted
  // and keep the ones that should survive.
  for (auto &node : nodes) {
    node.will_be_deleted = node.orphaned;
  }
  bool changed = true;
  while (changed) {
    changed = false;
    for (auto &node : nodes) {
      if (!node.will_be_deleted) {
        continue;
      }
      const bool goes = std::ranges::all_of(pool_.view(node.input_head), flagged) &&
          node.handle->audioNode->canBeDestructed();
      if (!goes) {
        node.will_be_deleted = false;
        changed = true;
      }
    }
  }

  for (auto &node : nodes) {
    if (!node.will_be_deleted) {
      pool_.removeIf(node.input_head, flagged);
    }
  }
}

void AudioGraph::remapListsToTargetIndex() {
  // Must run BEFORE nodes move: once they do, an index stored in a list no
  // longer names the node it was written for.
  for (auto &node : nodes) {
    for (auto &inputVertex : pool_.mutableView(node.input_head)) {
      inputVertex = static_cast<std::uint32_t>(nodes[inputVertex].target_place);
    }
  }
}

void AudioGraph::sortAndCompact() {
  if (topo_order_dirty) {
    topo_order_dirty = false;
    kahn_toposort();
  }

  // Only orphaned nodes can be deleted, so with none present skip the rest of the function.
  if (std::ranges::none_of(nodes, [](const Vertex &node) { return node.orphaned; })) {
    return;
  }

  markDeletions();

  // ── Assign each survivor its post-compaction position ───────────────────
  // Deleted nodes keep target_place == -1 and give their pool slots back now,
  // so the remap below never has to special-case them.
  std::uint32_t new_pos = 0;
  for (auto &node : nodes) {
    if (node.will_be_deleted) {
      pool_.freeAll(node.input_head);
    } else {
      node.target_place = static_cast<std::int32_t>(new_pos++);
    }
  }

  remapListsToTargetIndex();

  // ── Pass 2b: compact — shift kept nodes left ───────────────────────────
  std::uint32_t b = 0;
  const auto n = static_cast<std::uint32_t>(nodes.size());
  for (std::uint32_t e = 0; e < n; e++) {
    if (nodes[e].will_be_deleted) {
      continue;
    }
    if (b != e) {
      nodes[b] = std::move(nodes[e]);
    }
    nodes[b].handle->index_ = b;
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
    node.target_place = -1;
    node.will_be_deleted = false;
  }
}

void AudioGraph::settleProcessableState() {
  using PS = GraphObject::PROCESSABLE_STATE;

  std::int32_t top = -1;
  auto push = [&](std::uint32_t i) {
    nodes[i].target_place = top;
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
    // pop
    const auto idx = static_cast<std::uint32_t>(top);
    top = nodes[idx].target_place;
    nodes[idx].target_place = -1;

    for (const auto dep : pool_.view(nodes[idx].input_head)) {
      auto &obj = *nodes[dep].handle->audioNode;
      if (obj.processableState_ == PS::NOT_PROCESSABLE && !obj.excludeFromProcessablePull_) {
        obj.processableState_ = PS::CONDITIONAL_PROCESSABLE;
        push(dep);
      }
    }
  }
}

void AudioGraph::advanceUnpulledNodes(std::size_t currentSampleFrame) {
  for (auto &node : nodes) {
    auto &graphObject = *node.handle->audioNode;
    graphObject.rendersThisQuantum_ = graphObject.isProcessable();
    if (!graphObject.rendersThisQuantum_) {
      graphObject.advanceWithoutRender(currentSampleFrame);
    }
  }
}

// ── Kahn's toposort ───────────────────────────────────────────────────────

bool AudioGraph::breaksCycle(const Vertex &node) {
  const GraphObject *graphObject = node.handle->audioNode.get();
  return graphObject != nullptr && graphObject->breaksCycle_;
}

void AudioGraph::kahn_toposort() {
  const auto n = static_cast<std::uint32_t>(nodes.size());
  if (n <= 1) {
    return;
  }

  // Phase 1: compute out-degree
  for (const auto &nd : nodes) {
    if (breaksCycle(nd)) {
      continue;
    }
    for (const auto inp : pool_.view(nd.input_head)) {
      nodes[inp].topo_out_degree++;
    }
  }

  // Phase 2: reverse Kahn — sinks first, sources last in pop order.
  std::int32_t top = -1;
  auto push = [&](std::uint32_t i) {
    nodes[i].target_place = top; // temporary: link to the node below on the ready stack
    top = static_cast<std::int32_t>(i);
  };

  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].topo_out_degree == 0) {
      push(i);
    }
  }

  // sinks are processed first, thus we start with n so they end up last
  std::uint32_t write = n;
  while (top != -1) {
    // pop
    const auto idx = static_cast<std::uint32_t>(top);
    top = nodes[idx].target_place;
    nodes[idx].target_place = static_cast<std::int32_t>(--write); // final: position after the sort

    if (breaksCycle(nodes[idx])) {
      continue;
    }
    for (const auto inp : pool_.view(nodes[idx].input_head)) {
      if (--nodes[inp].topo_out_degree == 0) {
        push(inp);
      }
    }
  }
  assert(write == 0 && "a cycle exists");

  // Phase 3: remap input indices to new positions (before nodes move)
  remapListsToTargetIndex();

  // Phase 4: apply permutation in place via cycle sort
  for (std::uint32_t i = 0; i < n; i++) {
    while (nodes[i].target_place != static_cast<std::int32_t>(i)) {
      const auto t = static_cast<std::uint32_t>(nodes[i].target_place);
      std::swap(nodes[i], nodes[t]);
    }
  }

  // Phase 5: update handle indices & reset scratch
  for (std::uint32_t i = 0; i < n; i++) {
    if (nodes[i].handle) {
      nodes[i].handle->index_ = i;
    }
    nodes[i].target_place = -1;
  }
}

AudioGraph::NodeBuffer AudioGraph::adoptNodeBuffer(NodeBuffer preAllocated) {
  // Move live nodes into the pre-allocated (empty, large-capacity) buffer.
  // No reallocation: preAllocated.data.capacity() >= nodes.size() guaranteed
  // by the main thread before sending this event.
  std::ranges::move(nodes, std::back_inserter(preAllocated.data));
  std::swap(nodes, preAllocated.data);
  // preAllocated.data now holds the old (small) buffer with moved-from nodes.
  // Caller disposes it off the audio thread.
  return preAllocated;
}

} // namespace audioapi::utils::graph
