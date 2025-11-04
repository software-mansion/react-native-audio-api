#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/Locker.h>

namespace audioapi {

AudioNodeManager::Event::Event(Event &&other) {
  *this = std::move(other);
}

AudioNodeManager::Event &AudioNodeManager::Event::operator=(Event &&other) {
  if (this != &other) {
    // Clean up current resources
    this->~Event();

    // Move resources from the other event
    type = other.type;
    payloadType = other.payloadType;
    switch (payloadType) {
      case EventPayloadType::NODES:
        payload.nodes.from = std::move(other.payload.nodes.from);
        payload.nodes.to = std::move(other.payload.nodes.to);
        break;
      case EventPayloadType::PARAMS:
        payload.params.from = std::move(other.payload.params.from);
        payload.params.to = std::move(other.payload.params.to);
        break;
      case EventPayloadType::SOURCE_NODE:
        payload.sourceNode = std::move(other.payload.sourceNode);
        break;
      case EventPayloadType::AUDIO_PARAM:
        payload.audioParam = std::move(other.payload.audioParam);
        break;
      case EventPayloadType::NODE:
        payload.node = std::move(other.payload.node);
        break;

      default:
        break;
    }
  }
  return *this;
}

AudioNodeManager::Event::~Event() {
  switch (payloadType) {
    case EventPayloadType::NODES:
      payload.nodes.from.~shared_ptr();
      payload.nodes.to.~shared_ptr();
      break;
    case EventPayloadType::PARAMS:
      payload.params.from.~shared_ptr();
      payload.params.to.~shared_ptr();
      break;
    case EventPayloadType::SOURCE_NODE:
      payload.sourceNode.~shared_ptr();
      break;
    case EventPayloadType::AUDIO_PARAM:
      payload.audioParam.~shared_ptr();
      break;
    case EventPayloadType::NODE:
      payload.node.~shared_ptr();
      break;
  }
}

AudioNodeManager::AudioNodeManager() {
  sourceNodes_.reserve(kInitialCapacity);
  processingNodes_.reserve(kInitialCapacity);
  audioParams_.reserve(kInitialCapacity);

  auto channel_pair = channels::spsc::channel<
      std::unique_ptr<Event>,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::BUSY_LOOP>(kChannelCapacity);

  sender_ = std::move(channel_pair.first);
  receiver_ = std::move(channel_pair.second);
}

AudioNodeManager::~AudioNodeManager() {
  cleanup();
}

void AudioNodeManager::addPendingNodeConnection(
    const std::shared_ptr<AudioNode> &from,
    const std::shared_ptr<AudioNode> &to,
    ConnectionType type) {
  auto event = std::make_unique<Event>();
  event->type = type;
  event->payloadType = EventPayloadType::NODES;
  event->payload.nodes.from = from;
  event->payload.nodes.to = to;

  sender_.send(std::move(event));
}

void AudioNodeManager::addPendingParamConnection(
    const std::shared_ptr<AudioNode> &from,
    const std::shared_ptr<AudioParam> &to,
    ConnectionType type) {
  auto event = std::make_unique<Event>();
  event->type = type;
  event->payloadType = EventPayloadType::PARAMS;
  event->payload.params.from = from;
  event->payload.params.to = to;

  sender_.send(std::move(event));
}

void AudioNodeManager::preProcessGraph() {
  settlePendingConnections();
  prepareNodesForDestruction(sourceNodes_);
  prepareNodesForDestruction(processingNodes_);
}

void AudioNodeManager::addProcessingNode(
    const std::shared_ptr<AudioNode> &node) {
  auto event = std::make_unique<Event>();
  event->type = ConnectionType::ADD;
  event->payloadType = EventPayloadType::NODE;
  event->payload.node = node;

  sender_.send(std::move(event));
}

void AudioNodeManager::addSourceNode(
    const std::shared_ptr<AudioScheduledSourceNode> &node) {
  auto event = std::make_unique<Event>();
  event->type = ConnectionType::ADD;
  event->payloadType = EventPayloadType::SOURCE_NODE;
  event->payload.sourceNode = node;

  sender_.send(std::move(event));
}

void AudioNodeManager::addAudioParam(const std::shared_ptr<AudioParam> &param) {
  auto event = std::make_unique<Event>();
  event->type = ConnectionType::ADD;
  event->payloadType = EventPayloadType::AUDIO_PARAM;
  event->payload.audioParam = param;

  sender_.send(std::move(event));
}

void AudioNodeManager::settlePendingConnections() {
  std::unique_ptr<Event> value;
  while (receiver_.try_receive(value) !=
         channels::spsc::ResponseStatus::CHANNEL_EMPTY) {
    switch (value->type) {
      case ConnectionType::CONNECT:
        handleConnectEvent(std::move(value));
        break;
      case ConnectionType::DISCONNECT:
        handleDisconnectEvent(std::move(value));
        break;
      case ConnectionType::DISCONNECT_ALL:
        handleDisconnectAllEvent(std::move(value));
        break;
      case ConnectionType::ADD:
        handleAddToDeconstructionEvent(std::move(value));
        break;
    }
  }
}

void AudioNodeManager::handleConnectEvent(std::unique_ptr<Event> event) {
  if (event->payloadType == EventPayloadType::NODES) {
    event->payload.nodes.from->connectNode(event->payload.nodes.to);
  } else if (event->payloadType == EventPayloadType::PARAMS) {
    event->payload.params.from->connectParam(event->payload.params.to);
  } else {
    assert(false && "Invalid payload type for connect event");
  }
}

void AudioNodeManager::handleDisconnectEvent(std::unique_ptr<Event> event) {
  if (event->payloadType == EventPayloadType::NODES) {
    event->payload.nodes.from->disconnectNode(event->payload.nodes.to);
  } else if (event->payloadType == EventPayloadType::PARAMS) {
    event->payload.params.from->disconnectParam(event->payload.params.to);
  } else {
    assert(false && "Invalid payload type for disconnect event");
  }
}

void AudioNodeManager::handleDisconnectAllEvent(std::unique_ptr<Event> event) {
  assert(event->payloadType == EventPayloadType::NODES);
  for (auto it = event->payload.nodes.from->outputNodes_.begin();
       it != event->payload.nodes.from->outputNodes_.end();) {
    auto next = std::next(it);
    event->payload.nodes.from->disconnectNode(*it);
    it = next;
  }
}

void AudioNodeManager::handleAddToDeconstructionEvent(
    std::unique_ptr<Event> event) {
  switch (event->payloadType) {
    case EventPayloadType::NODE:
      processingNodes_.push_back(event->payload.node);
      break;
    case EventPayloadType::SOURCE_NODE:
      sourceNodes_.push_back(event->payload.sourceNode);
      break;
    case EventPayloadType::AUDIO_PARAM:
      audioParams_.push_back(event->payload.audioParam);
      break;
    default:
      assert(false && "Unknown event payload type");
  }
}

template <typename U>
inline bool AudioNodeManager::nodeCanBeDestructed(
    std::shared_ptr<U> const &node) {
  // If the node is an AudioScheduledSourceNode, we need to check if it is
  // playing
  if constexpr (std::is_base_of_v<AudioScheduledSourceNode, U>) {
    return node.use_count() == 1 &&
        (node->isUnscheduled() || node->isFinished());
  } else if constexpr (std::is_base_of_v<
                           ConvolverNode,
                           U>) { // convolver overrides disabling behavior
    return node.use_count() == 1 && !node->isEnabled();
  }
  return node.use_count() == 1;
}

template <typename U>
void AudioNodeManager::prepareNodesForDestruction(
    std::vector<std::shared_ptr<U>> &vec) {
  if (vec.empty()) {
    return;
  }
  /// An example of input-output
  /// for simplicity we will be considering vector where each value represents
  /// use_count() of an element vec = [1, 2, 1, 3, 1] our end result will be vec
  /// = [2, 3, 1, 1, 1] After this operation all nodes with use_count() == 1
  /// will be at the end and we will try to send them After sending, we will
  /// only keep nodes with use_count() > 1 or which failed vec = [2, 3, failed,
  /// sent, sent] // failed will be always before sents vec = [2, 3, failed] and
  /// we resize
  /// @note if there are no nodes with use_count() == 1 `begin` will be equal to
  /// vec.size()
  /// @note if all nodes have use_count() == 1 `begin` will be 0

  int begin = 0;
  int end = vec.size() - 1; // can be -1 (edge case)

  // Moves all nodes with use_count() == 1 to the end
  // nodes in range [begin, vec.size()) should be deleted
  // so new size of the vector will be `begin`
  while (begin <= end) {
    while (begin < end && AudioNodeManager::nodeCanBeDestructed(vec[end])) {
      end--;
    }
    if (AudioNodeManager::nodeCanBeDestructed(vec[begin])) {
      std::swap(vec[begin], vec[end]);
      end--;
    }
    begin++;
  }

  for (int i = begin; i < vec.size(); i++) {
    if (vec[i])
      vec[i]->cleanup();

    /// If we fail to add we can't safely remove the node from the vector
    /// so we swap it and advance begin cursor
    /// @note vec[i] does NOT get moved out if it is not successfully added.
    if (!nodeDeconstructor_.tryAddNodeForDeconstruction(std::move(vec[i]))) {
      std::swap(vec[i], vec[begin]);
      begin++;
    }
  }
  if (begin < vec.size()) {
    // it does not realocate if newer size is < current size
    vec.resize(begin);
  }
}

void AudioNodeManager::cleanup() {
  for (auto it = sourceNodes_.begin(), end = sourceNodes_.end(); it != end;
       ++it) {
    it->get()->cleanup();
  }

  for (auto it = processingNodes_.begin(), end = processingNodes_.end();
       it != end;
       ++it) {
    it->get()->cleanup();
  }

  sourceNodes_.clear();
  processingNodes_.clear();
  audioParams_.clear();
}

} // namespace audioapi
