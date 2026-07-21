#include <audioapi/core/AudioContext.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/analysis/AnalyserNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/GraphObject.h>
#include <audioapi/core/utils/graph/HostGraph.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <algorithm>

#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

namespace {

// Channel-layout vocabulary in this codebase:
//   downstream — away from AudioDestinationNode (toward sources); node->inputs
//   upstream   — toward AudioDestinationNode; node->outputs

/// @brief Returns the AudioNode associated with `node`, or nullptr if the
/// node has no audio payload (e.g. the lightweight `graph->addNode()` used
/// in tests that exercise topology only).
inline void notifyMediaElementOutputsDisconnected(
    audioapi::AudioNode *fromAudio,
    const HostGraph::Node *from) {
  if (from == nullptr || !from->outputs.empty() || fromAudio == nullptr) {
    return;
  }
  if (auto *media = dynamic_cast<audioapi::MediaElementAudioSourceNode *>(fromAudio)) {
    media->onOutputsDisconnected();
  }
}

inline audioapi::AudioNode *audioNodeOf(const HostGraph::Node *node) {
  if (node == nullptr || node->handle == nullptr || node->handle->audioNode == nullptr) {
    return nullptr;
  }
  return node->handle->audioNode->asAudioNode();
}

/// @brief Returns how many channels `audio` presents on upstream connections
/// (toward AudioDestinationNode). Uses the output buffer when present.
size_t outputChannelCountOf(const audioapi::AudioNode *audio) {
  if (audio == nullptr) {
    return 0;
  }
  const auto out = audio->getOutputBuffer();
  if (out != nullptr) {
    return out->getNumberOfChannels();
  }
  return audio->getChannelCount();
}

/// @brief Computes the channel count that `dest`'s negotiated buffer must carry
/// after the current set of inputs (`dest->inputs`). Follows the Web Audio
/// rules for `channelCountMode`:
///   - EXPLICIT     -> `channelCount` attribute (inputs ignored)
///   - MAX          -> max over inputs' computed output channel counts
///   - CLAMPED_MAX  -> min(channelCount attribute, max over inputs')
///
/// When there are no inputs the node keeps its own `channelCount` attribute
/// (matches the shape the buffer already had at construction time).
///
/// `term` identifies the current negotiation pass. Input nodes resolved in
/// that pass expose their pending upstream width via `channelLayout`.
size_t negotiateChannelCount(const HostGraph::Node *dest, size_t term) {
  auto *destAudio = audioNodeOf(dest);
  if (destAudio == nullptr) {
    return 0;
  }

  const auto attr = destAudio->getChannelCount();
  const auto mode = destAudio->getChannelCountMode();

  if (mode == audioapi::ChannelCountMode::EXPLICIT || dest->inputs.empty()) {
    return attr;
  }

  size_t maxInputChannels = 0;
  for (const HostGraph::Node *input : dest->inputs) {
    auto *inAudio = audioNodeOf(input);
    if (inAudio == nullptr) {
      continue;
    }
    size_t c = 0;
    if (input->channelLayout.isResolvedFor(term)) {
      c = input->channelLayout.upstreamChannelCount;
    } else {
      c = outputChannelCountOf(inAudio);
    }
    maxInputChannels = std::max(c, maxInputChannels);
  }

  if (maxInputChannels == 0) {
    return attr;
  }

  if (mode == audioapi::ChannelCountMode::CLAMPED_MAX) {
    return std::min(attr, maxInputChannels);
  }
  return maxInputChannels;
}

/// @brief If `dest` needs a newly negotiated channel layout, allocates a
/// replacement `DSPAudioBuffer` on the host thread and returns it.
/// Returns `nullptr` when no swap is needed (no audio payload, negotiation
/// converged, or context gone).
std::shared_ptr<audioapi::DSPAudioBuffer> buildNegotiatedBufferIfNeeded(
    const HostGraph::Node *dest,
    size_t desired) {
  auto *destAudio = audioNodeOf(dest);
  if (destAudio == nullptr) {
    return nullptr;
  }

  if (desired == 0) {
    return nullptr;
  }

  const auto current = destAudio->getNegotiatedBuffer();
  if (current != nullptr && current->getNumberOfChannels() == desired) {
    return nullptr;
  }

  return std::make_shared<audioapi::DSPAudioBuffer>(
      audioapi::RENDER_QUANTUM_SIZE, static_cast<int>(desired), destAudio->getContextSampleRate());
}

struct ChannelNegotiation {
  HostGraph::Node *node = nullptr;
  std::shared_ptr<audioapi::DSPAudioBuffer> buffer;
};

using NegotiationBatch = std::vector<ChannelNegotiation>;

/// @brief Recursively resolves upstream channel counts for `node` and every
/// downstream ancestor (`node->inputs`) in negotiation pass `term`.
void resolveChannelCountForNode(HostGraph::Node *node, size_t term) {
  if (node == nullptr || node->channelLayout.isResolvedFor(term)) {
    return;
  }

  for (HostGraph::Node *input : node->inputs) {
    resolveChannelCountForNode(input, term);
  }

  auto *audio = audioNodeOf(node);
  if (audio == nullptr) {
    // Stamp visited for non-AudioNode payloads (e.g. BridgeNode) so shared DAG
    // paths cannot re-enter this node in the same negotiation pass.
    node->channelLayout.setResolved(term, 0);
    return;
  }

  const size_t desired = negotiateChannelCount(node, term);
  node->channelLayout.setResolved(term, audio->getUpstreamChannelCount(desired));
}

/// @brief Starting at `dest` (the connect `to` node), negotiates channel
/// layouts for `dest` and every node further upstream (toward
/// AudioDestinationNode via `dest->outputs`).
void collectChannelNegotiations(HostGraph::Node *dest, size_t term, NegotiationBatch &out) {
  if (dest == nullptr || dest->channelLayout.isResolvedFor(term)) {
    return;
  }

  for (HostGraph::Node *input : dest->inputs) {
    resolveChannelCountForNode(input, term);
  }

  if (auto *destAudio = audioNodeOf(dest)) {
    const size_t desired = negotiateChannelCount(dest, term);
    dest->channelLayout.setResolved(term, destAudio->getUpstreamChannelCount(desired));

    if (auto negotiatedBuffer = buildNegotiatedBufferIfNeeded(dest, desired)) {
      out.push_back({.node = dest, .buffer = std::move(negotiatedBuffer)});
    }
  } else {
    dest->channelLayout.setResolved(term, 0);
  }

  for (HostGraph::Node *upstream : dest->outputs) {
    collectChannelNegotiations(upstream, term, out);
  }
}

void applyChannelNegotiations(
    const NegotiationBatch &negotiations,
    Disposer<audioapi::DISPOSER_PAYLOAD_SIZE> &disposer) {
  for (const ChannelNegotiation &negotiation : negotiations) {
    auto *audio = audioNodeOf(negotiation.node);
    if (audio == nullptr || negotiation.buffer == nullptr) {
      continue;
    }
    auto oldBuffer = audio->getNegotiatedBuffer();
    audio->setNegotiatedBuffer(negotiation.buffer);
    if (oldBuffer) {
      disposer.dispose(std::move(oldBuffer));
    }
  }
}

/// @brief Builds a fresh negotiation batch by cascading upstream from each
/// destination in `destinations`. All destinations share the same `term`, so a
/// node reachable from several of them is negotiated at most once per pass.
/// The batch is heap-allocated so ownership can be moved into the AGEvent and
/// later disposed off the audio thread.
std::unique_ptr<NegotiationBatch> collectNegotiations(
    size_t term,
    std::span<HostGraph::Node *const> destinations) {
  auto negotiations = std::make_unique<NegotiationBatch>();
  for (HostGraph::Node *dest : destinations) {
    collectChannelNegotiations(dest, term, *negotiations);
  }
  return negotiations;
}

/// @brief Single-destination convenience overload.
std::unique_ptr<NegotiationBatch> collectNegotiations(size_t term, HostGraph::Node *dest) {
  return collectNegotiations(term, std::span<HostGraph::Node *const>(&dest, 1));
}

} // namespace

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
    outs.erase(std::ranges::remove(outs, this).begin(), outs.end());
  }
  for (Node *output : outputs) {
    auto &inps = output->inputs;
    inps.erase(std::ranges::remove(inps, this).begin(), inps.end());
  }
}

HostGraph::HostGraph() = default;

HostGraph::~HostGraph() {
  std::scoped_lock lock(nodesMutex_);
  for (Node *n : nodes) {
    n->linkedNodes.clear();
  }
  for (Node *n : nodes) {
    delete n;
  }
  nodes.clear();
}

HostGraph::HostGraph(HostGraph &&other) noexcept
    : nodes(std::move(other.nodes)),
      edgeCount_(other.edgeCount_),
      linkCount_(other.linkCount_),
      last_term(other.last_term),
      channelLayoutTerm_(other.channelLayoutTerm_) {
  other.edgeCount_ = 0;
  other.linkCount_ = 0;
  other.last_term = 0;
  other.channelLayoutTerm_ = 0;
}

auto HostGraph::operator=(HostGraph &&other) noexcept -> HostGraph & {
  if (this != &other) {
    std::scoped_lock lock(nodesMutex_, other.nodesMutex_);
    for (Node *n : nodes) {
      n->linkedNodes.clear();
    }
    for (Node *n : nodes) {
      delete n;
    }
    nodes = std::move(other.nodes);
    edgeCount_ = other.edgeCount_;
    linkCount_ = other.linkCount_;
    last_term = other.last_term;
    channelLayoutTerm_ = other.channelLayoutTerm_;
    other.edgeCount_ = 0;
    other.linkCount_ = 0;
    other.last_term = 0;
    other.channelLayoutTerm_ = 0;
  }
  return *this;
}

auto HostGraph::addNode(std::shared_ptr<NodeHandle> handle) -> std::pair<Node *, AGEvent> {
  std::scoped_lock lock(nodesMutex_);
  Node *newNode = new Node();
  newNode->handle = handle;
  nodes.push_back(newNode);

  auto event = [h = std::move(handle)](auto &graph, auto &) mutable {
    graph.addNode(std::move(h));
  };

  return {newNode, std::move(event)};
}

auto HostGraph::removeNode(Node *node) -> Res {
  std::scoped_lock lock(nodesMutex_);
  auto it = std::ranges::find(nodes, node);
  if (it == nodes.end()) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  node->ghost = true;

  return Res::Ok(
      [h = node->handle](AudioGraph &graph, auto &) mutable { graph[h->index].orphaned = true; });
}

auto HostGraph::addEdge(Node *from, Node *to) -> Res {
  std::scoped_lock lock(nodesMutex_);
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
  const auto inputCount = static_cast<std::uint32_t>(to->inputs.size());
  edgeCount_++;

  // Channel-count negotiation: computed + allocated on the host thread,
  // applied on the audio thread by the AGEvent below. Cascade upstream
  // (toward AudioDestinationNode) so late downstream connects still propagate.
  auto negotiations = collectNegotiations(++channelLayoutTerm_, to);

  return Res::Ok(
      [hTo = to->handle, hFrom = from->handle, inputCount, negotiations = std::move(negotiations)](
          AudioGraph &graph, auto &disposer) mutable {
        applyChannelNegotiations(*negotiations, disposer);
        disposer.dispose(std::move(negotiations));
        hTo->audioNode->inputBuffers_.reserve(inputCount);
        graph.pool().push(graph[hTo->index].input_head, hFrom->index);
        graph.markDirty();
      });
}

auto HostGraph::removeEdge(Node *from, Node *to) -> Res {
  std::scoped_lock lock(nodesMutex_);
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

  if (auto *fromAudio = from->handle->audioNode->asAudioNode()) {
    notifyMediaElementOutputsDisconnected(fromAudio, from);
  }

  // Channel-count negotiation: computed + allocated on the host thread,
  // applied on the audio thread by the AGEvent below. Cascade upstream
  // (toward AudioDestinationNode) so disconnects still propagate.
  auto negotiations = collectNegotiations(++channelLayoutTerm_, to);

  return Res::Ok([hFrom = from->handle, hTo = to->handle, negotiations = std::move(negotiations)](
                     AudioGraph &graph, auto &disposer) mutable {
    applyChannelNegotiations(*negotiations, disposer);
    disposer.dispose(std::move(negotiations));
    graph.pool().remove(graph[hTo->index].input_head, hFrom->index);
    graph.markDirty();
  });
}

auto HostGraph::removeAllEdges(Node *from) -> Res {
  std::scoped_lock lock(nodesMutex_);
  if (std::ranges::find(nodes, from) == nodes.end() || from->ghost) {
    return Res::Err(ResultError::NODE_NOT_FOUND);
  }

  auto pairs = std::vector<std::pair<std::uint32_t, std::uint32_t>>();
  pairs.reserve(from->outputs.size());
  auto formerOutputs = std::vector<Node *>();
  formerOutputs.reserve(from->outputs.size());

  for (Node *to : from->outputs) {
    auto itIn = std::ranges::find(to->inputs, from);
    if (itIn != to->inputs.end()) {
      to->inputs.erase(itIn);
    }
    edgeCount_--;
    pairs.emplace_back(from->handle->index, to->handle->index);
    formerOutputs.push_back(to);
  }
  from->outputs.clear();

  if (auto *fromAudio = from->handle->audioNode->asAudioNode()) {
    notifyMediaElementOutputsDisconnected(fromAudio, from);
  }

  // Channel-count negotiation: computed + allocated on the host thread,
  // applied on the audio thread by the AGEvent below. Cascade upstream from
  // each former output (toward AudioDestinationNode) so every downstream node
  // that lost this input re-derives its layout.
  auto negotiations = collectNegotiations(++channelLayoutTerm_, formerOutputs);

  return Res::Ok([pairs = std::move(pairs), negotiations = std::move(negotiations)](
                     AudioGraph &graph, auto &disposer) mutable {
    applyChannelNegotiations(*negotiations, disposer);
    disposer.dispose(std::move(negotiations));
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

std::optional<HostGraph::AGEvent> HostGraph::linkNodes(Node *from, Node *to) {
  std::scoped_lock lock(nodesMutex_);
  if (from == nullptr || to == nullptr || from == to) {
    return std::nullopt;
  }
  if (from->ghost || to->ghost) {
    return std::nullopt;
  }
  if (std::ranges::find(from->linkedNodes, to) != from->linkedNodes.end()) {
    return std::nullopt;
  }
  from->linkedNodes.push_back(to);
  linkCount_++;

  return AGEvent([hFrom = from->handle, hTo = to->handle](AudioGraph &graph, auto &) mutable {
    graph.pool().push(graph[hFrom->index].link_head, hTo->index);
    graph.markDirty();
  });
}

size_t HostGraph::edgeCount() const {
  std::scoped_lock lock(nodesMutex_);
  return edgeCount_;
}

size_t HostGraph::linkCount() const {
  std::scoped_lock lock(nodesMutex_);
  return linkCount_;
}

size_t HostGraph::nodeCount() const {
  std::scoped_lock lock(nodesMutex_);
  return nodes.size();
}

void HostGraph::collectDisposedNodes() {
  std::scoped_lock lock(nodesMutex_);
  for (auto it = nodes.begin(); it != nodes.end();) {
    Node *n = *it;
    if (n->ghost && n->handle.use_count() == 1) {
      edgeCount_ -= n->outputs.size();
      // Outgoing links from n, plus any incoming links to n from other nodes,
      // are all being torn down — keep linkCount_ in sync for pool sizing.
      linkCount_ -= n->linkedNodes.size();
      for (Node *m : nodes) {
        auto &ln = m->linkedNodes;
        auto newEnd = std::ranges::remove(ln, n).begin();
        linkCount_ -= static_cast<size_t>(std::distance(newEnd, ln.end()));
        ln.erase(newEnd, ln.end());
      }
      *it = nodes.back();
      nodes.pop_back();
      delete n;
    } else {
      ++it;
    }
  }
}

} // namespace audioapi::utils::graph
