#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/Locker.h>

namespace audioapi {

AudioNodeManager::~AudioNodeManager() {
  Locker lock(getGraphLock());

  for (auto it = nodes_.begin(); it != nodes_.end();) {
    it->get()->cleanup();
  }

  nodes_.clear();
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
  for (auto it = audioNodesToConnect_.begin(); it != audioNodesToConnect_.end();) {
    std::shared_ptr<AudioNode> from = std::get<0>(*it);
    std::shared_ptr<AudioNode> to = std::get<1>(*it);
    ConnectionType type = std::get<2>(*it);

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
  printf("nodes count: %zu\n", nodes_.size());
  auto it = nodes_.begin();

  while (it != nodes_.end()) {
    if (it->use_count == 1) {
      it->get()->cleanup();
      it = nodes_.erase(it);
    } else {
      ++it;
    }
  }
}

void AudioNodeManager::cleanup() {
  Locker lock(getGraphLock());

  for (auto it = nodes_.begin(); it != nodes_.end();) {
    it->get()->cleanup();
  }

  nodes_.clear();
}

} // namespace audioapi
