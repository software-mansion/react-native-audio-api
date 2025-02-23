
#include "AudioNodeManager.h"
#include "AudioNode.h"
#include "Locker.h"

namespace audioapi {

AudioNodeManager::~AudioNodeManager() {
  audioNodesToConnect_.clear();
}

void AudioNodeManager::addPendingConnection(
    const std::shared_ptr<AudioNode> &from,
    const std::shared_ptr<AudioNode> &to,
    ConnectionType type) {
  Locker lock(getGraphLock());

  audioNodesToConnect_.emplace_back(from, to, type);
}

void AudioNodeManager::preProcessGraph() {
  if (!Locker::tryLock(getGraphLock())) {
    return;
  }

  settlePendingConnections();
  prepareNodesForDestruction();
}

std::mutex &AudioNodeManager::getGraphLock() {
  return graphLock_;
}

void AudioNodeManager::addNode(const std::shared_ptr<AudioNode> &node) {
  Locker lock(getGraphLock());

  nodes_.insert(node);
}

void AudioNodeManager::settlePendingConnections() {
  for (auto &connection : audioNodesToConnect_) {
    std::shared_ptr<AudioNode> from = std::get<0>(connection);
    std::shared_ptr<AudioNode> to = std::get<1>(connection);
    ConnectionType type = std::get<2>(connection);

    if (!to || !from) {
      continue;
    }

    if (type == ConnectionType::CONNECT) {
      from->connectNode(to);
    } else {
      from->disconnectNode(to);
    }
  }

  audioNodesToConnect_.clear();
}

void AudioNodeManager::prepareNodesForDestruction() {
  for (auto it = nodes_.begin(); it != nodes_.end();) {
    if (it->use_count() == 1) {
      it->get()->cleanup();
      it = nodes_.erase(it);
    } else {
      ++it;
    }
  }
}

void AudioNodeManager::cleanup() {
  Locker lock(getGraphLock());

  for (auto &node : nodes_) {
    node->cleanup();
  }

  nodes_.clear();
}

} // namespace audioapi
