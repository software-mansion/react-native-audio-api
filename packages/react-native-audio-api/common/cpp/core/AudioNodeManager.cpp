
#include "Locker.hpp"
#include "AudioNode.h"
#include "AudioNodeManager.h"

namespace audioapi {

AudioNodeManager::AudioNodeManager() {}

AudioNodeManager::~AudioNodeManager() {
  audioNodesToConnect_.clear();
  audioNodesToDelete_.clear();
}

void AudioNodeManager::addPendingConnection(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to, ConnectionType type) {
  Locker lock(getGraphLock());

  audioNodesToConnect_.push_back(std::make_tuple(from, to, type));
}

void AudioNodeManager::setNodeToDelete(std::shared_ptr<AudioNode> node) {
  Locker lock(getGraphLock());

  audioNodesToDelete_.push_back(node);
}

void AudioNodeManager::preProcessGraph() {
    if (!Locker::tryLock(getGraphLock())) {
    return;
  }
}

void AudioNodeManager::postProcessGraph() {
  if (!Locker::tryLock(getGraphLock())) {
    return;
  }
}

std::mutex& AudioNodeManager::getGraphLock() {
  return graphLock_;
}

void AudioNodeManager::settlePendingConnections() {
  for (auto& connection : audioNodesToConnect_) {
    std::shared_ptr<AudioNode> from = std::get<0>(connection);
    std::shared_ptr<AudioNode> to = std::get<1>(connection);
    ConnectionType type = std::get<2>(connection);

    if (type == ConnectionType::CONNECT) {
      from->connectNode(to);
    } else {
      from->disconnectNode(to);
    }
  }

  audioNodesToConnect_.clear();
}

void AudioNodeManager::settlePendingDeletions() {
  audioNodesToDelete_.clear();
}

} // namespace audioapi
