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

#include <cassert>
#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <ranges>
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
    const HostGraph::HostVertex *from) {
  if (from == nullptr || !from->outputs.empty() || fromAudio == nullptr) {
    return;
  }
  if (auto *media = dynamic_cast<audioapi::MediaElementAudioSourceNode *>(fromAudio)) {
    media->onOutputsDisconnected();
  }
}

inline audioapi::AudioNode *audioNodeOf(const HostGraph::HostVertex *node) {
  if (node == nullptr || node->handle == nullptr || node->handle->audioNode == nullptr) {
    return nullptr;
  }
  return node->handle->audioNode->asAudioNode();
}

/// @brief Width `node` will emit toward AudioDestinationNode once every
/// queued swap has landed: a fixed override (StereoPanner's stereo output, a
/// source's declared width) or else the width of its pending negotiated
/// buffer. 0 for payloads without an AudioNode (e.g. BridgeNode).
size_t pendingOutputChannelNumber(const HostGraph::HostVertex *node) {
  auto *audio = audioNodeOf(node);
  if (audio == nullptr) {
    return 0;
  }
  return audio->getOutputChannelNumber().value_or(node->pendingChannelCount);
}

/// @brief True when `node` is a source: it has no inputs to mix, so it owns
/// its output buffer and swaps it together with the content that defines its
/// width (f.e. AudioBufferSourceNode's buffer number of channels).
bool ownsOutputBuffer(const HostGraph::HostVertex *node) {
  auto *audio = audioNodeOf(node);
  return audio != nullptr && audio->getNumberOfInputs() == 0;
}

/// @brief Computes the spec's `computedNumberOfChannels` for `node`: the width
/// its inputs (`node->inputs`) are mixed to, which is also the width of its
/// negotiated buffer. Follows the Web Audio rules for `channelCountMode`:
///   - EXPLICIT     -> `channelCount` attribute (inputs ignored)
///   - MAX          -> max over inputs' output channel numbers
///   - CLAMPED_MAX  -> min(channelCount attribute, max over inputs')
///
/// When there are no inputs the node keeps its own `channelCount` attribute
/// (matches the shape the buffer already had at construction time).
size_t computeNumberOfChannels(const HostGraph::HostVertex *node) {
  auto *audio = audioNodeOf(node);
  if (audio == nullptr) {
    return 0;
  }

  const auto channelCount = audio->getChannelCount();
  const auto mode = audio->getChannelCountMode();

  if (mode == audioapi::ChannelCountMode::EXPLICIT || node->inputs.empty()) {
    return channelCount;
  }

  size_t maxInputChannels = 0;
  for (const HostGraph::HostVertex *input : node->inputs) {
    maxInputChannels = std::max(pendingOutputChannelNumber(input), maxInputChannels);
  }

  if (maxInputChannels == 0) {
    return channelCount;
  }

  if (mode == audioapi::ChannelCountMode::CLAMPED_MAX) {
    return std::min(channelCount, maxInputChannels);
  }
  return maxInputChannels;
}

/// @brief If `node` needs a newly negotiated channel layout, allocates a
/// replacement `DSPAudioBuffer` on the host thread, records its width as
/// pending, and returns it. Returns `nullptr` when no swap is needed (no audio
/// payload, or the pending buffer already has `computedNumberOfChannels`).
std::shared_ptr<audioapi::DSPAudioBuffer> buildNegotiatedBufferIfNeeded(
    HostGraph::HostVertex *node,
    size_t computedNumberOfChannels) {
  auto *audio = audioNodeOf(node);
  if (audio == nullptr) {
    return nullptr;
  }

  if (computedNumberOfChannels == 0 || node->pendingChannelCount == computedNumberOfChannels) {
    return nullptr;
  }
  node->pendingChannelCount = computedNumberOfChannels;

  return std::make_shared<audioapi::DSPAudioBuffer>(
      audio->getContextRenderQuantumSize(),
      static_cast<int>(computedNumberOfChannels),
      audio->getContextSampleRate());
}

struct ChannelNegotiation {
  HostGraph::HostVertex *node = nullptr;
  std::shared_ptr<audioapi::DSPAudioBuffer> buffer;
};

using NegotiationBatch = std::vector<ChannelNegotiation>;

/// @brief Appends `node` (one whose inputs or channel attributes changed, f.e.
/// the connect `to` node) and every node further upstream (toward
/// AudioDestinationNode via `node->outputs`) to `postOrder`, each one after
/// every node it feeds.
void collectAffectedPostOrder(
    HostGraph::HostVertex *node,
    size_t term,
    std::vector<HostGraph::HostVertex *> &postOrder) {
  if (node == nullptr || !node->channelLayoutTraversal.visit(term)) {
    return;
  }

  for (HostGraph::HostVertex *upstream : node->outputs) {
    collectAffectedPostOrder(upstream, term, postOrder);
  }
  postOrder.push_back(node);
}

// clang-tidy does not search deep enough to find the usage of this fun
// NOLINTNEXTLINE
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

/// @brief Builds a fresh negotiation batch by cascading upstream (toward
/// AudioDestinationNode) from each node in `changedNodes`: the nodes whose
/// inputs or channel attributes just changed. They all share the same `term`,
/// so a node reachable from several of them is negotiated once per pass.
std::unique_ptr<NegotiationBatch> collectNegotiations(
    size_t term,
    std::span<HostGraph::HostVertex *const> changedNodes) {
  std::vector<HostGraph::HostVertex *> postOrder;
  for (HostGraph::HostVertex *changedNode : changedNodes) {
    collectAffectedPostOrder(changedNode, term, postOrder);
  }

  // Reversed post-order is topological: a node reads its inputs' pending
  // widths only after every input this pass renegotiates has been updated,
  // whichever order the cascade reached them in (f.e. a triangle).
  auto negotiations = std::make_unique<NegotiationBatch>();
  for (HostGraph::HostVertex *node : std::views::reverse(postOrder)) {
    if (auto negotiatedBuffer =
            buildNegotiatedBufferIfNeeded(node, computeNumberOfChannels(node))) {
      negotiations->push_back({.node = node, .buffer = std::move(negotiatedBuffer)});
    }
  }
  return negotiations;
}

/// @brief Single-node convenience overload.
std::unique_ptr<NegotiationBatch> collectNegotiations(
    size_t term,
    HostGraph::HostVertex *changedNode) {
  return collectNegotiations(term, std::span<HostGraph::HostVertex *const>(&changedNode, 1));
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

HostGraph::HostVertex::~HostVertex() {
  for (HostVertex *input : inputs) {
    auto &outs = input->outputs;
    outs.erase(std::ranges::remove(outs, this).begin(), outs.end());
  }
  for (HostVertex *output : outputs) {
    auto &inps = output->inputs;
    inps.erase(std::ranges::remove(inps, this).begin(), inps.end());
  }
}

HostGraph::HostGraph() = default;

HostGraph::~HostGraph() {
  std::scoped_lock lock(nodesMutex_);
  for (HostVertex *n : nodes) {
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
    for (HostVertex *n : nodes) {
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

auto HostGraph::addNode(std::shared_ptr<NodeHandle> handle) -> std::pair<HostVertex *, AGEvent> {
  std::scoped_lock lock(nodesMutex_);
  HostVertex *newNode = new HostVertex();
  newNode->handle = handle;
  // AudioGraph learns about the node only from the event below, so the audio
  // thread cannot be swapping this buffer yet.
  if (auto *audio = audioNodeOf(newNode)) {
    if (auto negotiatedBuffer = audio->getNegotiatedBuffer()) {
      newNode->pendingChannelCount = negotiatedBuffer->getNumberOfChannels();
    }
  }
  nodes.push_back(newNode);

  auto event = [h = std::move(handle)](auto &graph, auto &) mutable {
    graph.addNode(std::move(h));
  };

  return {newNode, std::move(event)};
}

auto HostGraph::removeNode(HostVertex *node) -> Result<AGEvent, GraphError> {
  std::scoped_lock lock(nodesMutex_);
  auto it = std::ranges::find(nodes, node);
  if (it == nodes.end()) {
    return Err(GraphError::NODE_NOT_FOUND);
  }

  node->ghost = true;

  return Ok(
      [h = node->handle](AudioGraph &graph, auto &) mutable { graph[h->index].orphaned = true; });
}

auto HostGraph::addEdge(HostVertex *from, HostVertex *to) -> Result<AGEvent, GraphError> {
  std::scoped_lock lock(nodesMutex_);
  if (std::ranges::find(nodes, from) == nodes.end() ||
      std::ranges::find(nodes, to) == nodes.end()) {
    return Err(GraphError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
    return Err(GraphError::NODE_NOT_FOUND);
  }

  for (HostVertex *out : from->outputs) {
    if (out == to) {
      return Err(GraphError::EDGE_ALREADY_EXISTS);
    }
  }

  if (hasPath(to, from)) {
    return Err(GraphError::CYCLE_DETECTED);
  }

  // `connect()` already rejects a source as destination; negotiation below
  // would otherwise build a buffer narrower than the content the source
  // writes into it.
  assert(!ownsOutputBuffer(to));

  from->outputs.push_back(to);
  to->inputs.push_back(from);
  edgeCount_++;

  // Reserve the destination's input-scratch capacity off the audio thread.
  // `inputBuffers_` is audio-thread-only, so we allocate a replacement here and
  // swap it in from the AGEvent instead of mutating the live vector (which
  // `process()` reads concurrently on the audio thread — a data race).
  auto reservedInputs = std::make_unique<std::vector<const audioapi::DSPAudioBuffer *>>();
  reservedInputs->reserve(to->inputs.size());

  // Channel negotiation: computed + allocated on the host thread,
  // applied on the audio thread by the AGEvent below. Cascade upstream
  // (toward AudioDestinationNode) so late downstream connects still propagate.
  auto negotiations = collectNegotiations(++channelLayoutTerm_, to);

  return Ok(
      [hTo = to->handle,
       hFrom = from->handle,
       negotiations = std::move(negotiations),
       reservedInputs = std::move(reservedInputs)](AudioGraph &graph, auto &disposer) mutable {
        auto *toNode = hTo->audioNode.get();
        applyChannelNegotiations(*negotiations, disposer);
        disposer.dispose(std::move(negotiations));
        // dispose the vector
        disposer.dispose(toNode->exchangeInputScratch(std::move(*reservedInputs)));
        // dispose the unique_ptr
        disposer.dispose(std::move(reservedInputs));
        graph.pool().push(graph[hTo->index].input_head, hFrom->index);
        graph.markDirty();
      });
}

auto HostGraph::removeEdge(HostVertex *from, HostVertex *to) -> Result<AGEvent, GraphError> {
  std::scoped_lock lock(nodesMutex_);
  if (std::ranges::find(nodes, from) == nodes.end() ||
      std::ranges::find(nodes, to) == nodes.end()) {
    return Err(GraphError::NODE_NOT_FOUND);
  }
  if (from->ghost || to->ghost) {
    return Err(GraphError::NODE_NOT_FOUND);
  }

  auto itOut = std::ranges::find(from->outputs, to);
  if (itOut == from->outputs.end()) {
    return Err(GraphError::EDGE_NOT_FOUND);
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

  // Channel negotiation: computed + allocated on the host thread,
  // applied on the audio thread by the AGEvent below. Cascade upstream
  // (toward AudioDestinationNode) so disconnects still propagate.
  auto negotiations = collectNegotiations(++channelLayoutTerm_, to);

  return Ok([hFrom = from->handle, hTo = to->handle, negotiations = std::move(negotiations)](
                AudioGraph &graph, auto &disposer) mutable {
    applyChannelNegotiations(*negotiations, disposer);
    disposer.dispose(std::move(negotiations));
    graph.pool().remove(graph[hTo->index].input_head, hFrom->index);
    graph.markDirty();
  });
}

auto HostGraph::removeAllEdges(HostVertex *from) -> Result<AGEvent, GraphError> {
  std::scoped_lock lock(nodesMutex_);
  if (std::ranges::find(nodes, from) == nodes.end() || from->ghost) {
    return Err(GraphError::NODE_NOT_FOUND);
  }

  auto pairs = std::vector<std::pair<std::uint32_t, std::uint32_t>>();
  pairs.reserve(from->outputs.size());
  auto formerOutputs = std::vector<HostVertex *>();
  formerOutputs.reserve(from->outputs.size());

  for (HostVertex *to : from->outputs) {
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

  // Channel negotiation: computed + allocated on the host thread,
  // applied on the audio thread by the AGEvent below. Cascade upstream from
  // each former output (toward AudioDestinationNode) so every downstream node
  // that lost this input re-derives its layout.
  auto negotiations = collectNegotiations(++channelLayoutTerm_, formerOutputs);

  return Ok([pairs = std::move(pairs), negotiations = std::move(negotiations)](
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

auto HostGraph::renegotiateNodeChannels(HostVertex *node) -> Result<AGEvent, GraphError> {
  std::scoped_lock lock(nodesMutex_);
  if (node == nullptr || std::ranges::find(nodes, node) == nodes.end()) {
    return Err(GraphError::NODE_NOT_FOUND);
  }
  if (node->ghost) {
    return Err(GraphError::NODE_NOT_FOUND);
  }

  // Recompute channel layouts for this node and everything upstream. A source
  // owns its output buffer, so only its consumers have anything to renegotiate.
  auto negotiations = ownsOutputBuffer(node)
      ? collectNegotiations(++channelLayoutTerm_, node->outputs)
      : collectNegotiations(++channelLayoutTerm_, node);

  return Ok([negotiations = std::move(negotiations)](AudioGraph &graph, auto &disposer) mutable {
    applyChannelNegotiations(*negotiations, disposer);
    disposer.dispose(std::move(negotiations));
    graph.markDirty();
  });
}

bool HostGraph::hasPath(HostVertex *start, HostVertex *end) {
  if (start == end) {
    return true;
  }

  last_term++;
  size_t term = last_term;

  std::vector<HostVertex *> stack;
  stack.push_back(start);
  start->traversalState.term = term;

  while (!stack.empty()) {
    HostVertex *curr = stack.back();
    stack.pop_back();

    if (curr == end) {
      return true;
    }

    for (HostVertex *out : curr->outputs) {
      if (out->traversalState.visit(term)) {
        stack.push_back(out);
      }
    }
  }
  return false;
}

std::optional<HostGraph::AGEvent> HostGraph::linkNodes(HostVertex *from, HostVertex *to) {
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
    HostVertex *n = *it;
    if (n->ghost && n->handle.use_count() == 1) {
      edgeCount_ -= n->outputs.size();
      // Outgoing links from n, plus any incoming links to n from other nodes,
      // are all being torn down — keep linkCount_ in sync for pool sizing.
      linkCount_ -= n->linkedNodes.size();
      for (HostVertex *m : nodes) {
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
