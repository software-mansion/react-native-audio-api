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
#include <memory>
#include <utility>
#include <vector>

namespace audioapi::utils::graph {

namespace {

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

/// @brief Computes the channel count that `dest`'s output buffer must carry
/// after the current set of inputs (`dest->inputs`). Follows the Web Audio
/// rules for `channelCountMode`:
///   - EXPLICIT     -> `channelCount` attribute (inputs ignored)
///   - MAX          -> max over inputs' channel counts
///   - CLAMPED_MAX  -> min(channelCount attribute, max over inputs')
///
/// When there are no inputs the node keeps its own `channelCount` attribute
/// (matches the shape the buffer already had at construction time).
///
/// Reads only host-thread-owned state: the const `channelCountMode_` and
/// the `channelCount` attributes of the upstream nodes.
size_t negotiateChannelCount(const HostGraph::Node *dest) {
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
    const auto c = inAudio->getChannelCount();
    maxInputChannels = std::max(c, maxInputChannels);
  }

  if (maxInputChannels == 0) {
    return attr;
  }

  switch (mode) {
    case audioapi::ChannelCountMode::MAX:
      return maxInputChannels;
    case audioapi::ChannelCountMode::CLAMPED_MAX:
      return std::min(attr, maxInputChannels);
    case audioapi::ChannelCountMode::EXPLICIT:
      return attr;
  }
  return attr;
}

/// @brief If `dest` is a real AudioNode and the newly negotiated channel
/// count differs from what its output buffer already has, allocates a
/// replacement `DSPAudioBuffer` on the host thread and returns it.
/// Returns `nullptr` when no swap is needed (no audio payload, negotiation
/// converged, or context gone).
std::shared_ptr<audioapi::DSPAudioBuffer> buildNegotiatedBufferIfNeeded(
    const HostGraph::Node *dest) {
  auto *destAudio = audioNodeOf(dest);
  if (destAudio == nullptr) {
    return nullptr;
  }

  const size_t desired = negotiateChannelCount(dest);
  if (desired == 0) {
    return nullptr;
  }

  // The current buffer shape is the only authoritative "current state" for
  // channel count (the attribute can differ from the effective buffer in
  // MAX / CLAMPED_MAX modes). Reading `getNumberOfChannels()` here is a
  // plain load of a size_t owned by the shared_ptr we already hold, so it
  // is safe on the host thread as long as the buffer pointer itself is
  // only ever swapped under an AGEvent (which is the case).
  const auto current = destAudio->getOutputBuffer();
  if (current != nullptr && current->getNumberOfChannels() == desired) {
    return nullptr;
  }

  return std::make_shared<audioapi::DSPAudioBuffer>(
      audioapi::RENDER_QUANTUM_SIZE, static_cast<int>(desired), destAudio->getContextSampleRate());
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
    : nodes(std::move(other.nodes)), edgeCount_(other.edgeCount_), last_term(other.last_term) {
  other.edgeCount_ = 0;
  other.last_term = 0;
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
    last_term = other.last_term;
    other.edgeCount_ = 0;
    other.last_term = 0;
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

void HostGraph::markNodesAsProcessing(Node *node) {
  if (node == nullptr) {
    return;
  }
  // Already CONDITIONAL or ALWAYS — do not recurse. Stops infinite recursion
  // on cycles (e.g. delay feedback) and avoids redundant walks.
  if (node->handle->audioNode->isProcessable()) {
    return;
  }
  node->handle->audioNode->setProcessableState(
      GraphObject::PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE);

  for (Node *input : node->inputs) {
    markNodesAsProcessing(input);
  }
  for (Node *linked : node->linkedNodes) {
    markNodesAsProcessing(linked);
  }
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
  to->handle->audioNode->inputBuffers_.reserve(to->inputs.size());
  edgeCount_++;

  // Channel-count negotiation: computed + allocated on the host thread,
  // applied on the audio thread by the AGEvent below.
  auto negotiatedBuffer = buildNegotiatedBufferIfNeeded(to);

  // could be problematic, since we are passing raw pointer to the lambda
  return Res::Ok(
      [from,
       hTo = to->handle,
       hFrom = from->handle,
       negotiatedBuffer = std::move(negotiatedBuffer)](AudioGraph &graph, auto &disposer) mutable {
        auto *fromNode = hFrom->audioNode.get();
        auto *toNode = hTo->audioNode.get();
        if (!fromNode->isProcessable() && toNode->isProcessable()) {
          markNodesAsProcessing(from);
        }
        auto *toAudio = toNode->asAudioNode();
        if (toAudio != nullptr && negotiatedBuffer != nullptr) {
          auto oldBuffer = toAudio->getOutputBuffer();
          toAudio->setOutputBuffer(negotiatedBuffer);
          if (oldBuffer) {
            disposer.dispose(std::move(oldBuffer));
          }
        }
        graph.pool().push(graph[hTo->index].input_head, from->handle->index);
        graph.markDirty();
      });
}

void HostGraph::markNodesAsNotProcessing(Node *node) {
  if (node == nullptr || node->handle == nullptr || node->handle->audioNode == nullptr) {
    return;
  }
  auto *audioNode = node->handle->audioNode.get();
  if (!audioNode->isProcessable()) {
    return;
  }
  if (audioNode->processableState_ != GraphObject::PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE) {
    return;
  }
  audioNode->setProcessableState(GraphObject::PROCESSABLE_STATE::NOT_PROCESSABLE);

  for (Node *input : node->inputs) {
    markNodesAsNotProcessing(input);
  }
  for (Node *linked : node->linkedNodes) {
    markNodesAsNotProcessing(linked);
  }
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
  // applied on the audio thread by the AGEvent below.
  auto negotiatedBuffer = buildNegotiatedBufferIfNeeded(to);

  // could be problematic, since we are passing raw pointer to the lambda
  return Res::Ok([from,
                  hTo = to->handle,
                  hFrom = from->handle,
                  negotiatedBuffer = std::move(negotiatedBuffer)](
                     AudioGraph &graph, auto &disposer) mutable {
    auto *fromAudio = hFrom->audioNode.get();

    if (fromAudio->processableState_ == GraphObject::PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE) {
      bool updateProcessingNodes = std::ranges::all_of(from->outputs, [](Node *output) {
        auto *outAudio = output->handle->audioNode.get();
        return !outAudio->isProcessable();
      });
      if (updateProcessingNodes) {
        HostGraph::markNodesAsNotProcessing(from);
      }
    }
    if (auto *toAudio = hTo->audioNode->asAudioNode();
        toAudio != nullptr && negotiatedBuffer != nullptr) {
      auto oldBuffer = toAudio->getOutputBuffer();
      toAudio->setOutputBuffer(negotiatedBuffer);
      if (oldBuffer) {
        disposer.dispose(std::move(oldBuffer));
      }
    }
    graph.pool().remove(graph[hTo->index].input_head, from->handle->index);
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

  for (Node *to : from->outputs) {
    auto itIn = std::ranges::find(to->inputs, from);
    if (itIn != to->inputs.end()) {
      to->inputs.erase(itIn);
    }
    edgeCount_--;
    pairs.emplace_back(from->handle->index, to->handle->index);
  }
  from->outputs.clear();

  if (auto *fromAudio = from->handle->audioNode->asAudioNode()) {
    notifyMediaElementOutputsDisconnected(fromAudio, from);
  }

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

void HostGraph::linkNodes(Node *from, Node *to) {
  if (from == nullptr || to == nullptr || from == to) {
    return;
  }
  if (std::ranges::find(from->linkedNodes, to) != from->linkedNodes.end()) {
    return;
  }
  from->linkedNodes.push_back(to);
}

size_t HostGraph::edgeCount() const {
  std::scoped_lock lock(nodesMutex_);
  return edgeCount_;
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
      for (Node *m : nodes) {
        auto &ln = m->linkedNodes;
        ln.erase(std::ranges::remove(ln, n).begin(), ln.end());
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
