#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/Locker.h>

namespace audioapi {

AudioNodeManager::AudioNodeManager() {}

AudioNodeManager::~AudioNodeManager() {
  cleanup();
}

void AudioNodeManager::addPendingConnection(
    const std::shared_ptr<AudioNode> &from,
    const std::shared_ptr<AudioNode> &to,
    ConnectionType type) {
  Locker lock(getGraphLock());

  audioNodesToConnect_.emplace_back(from, to, type);
}

void AudioNodeManager::preProcessGraph() {
  if (auto locker = Locker::tryLock(getGraphLock())) {
    settlePendingConnections();
    prepareNodesForDestruction();
  }
}

std::mutex &AudioNodeManager::getGraphLock() {
  return graphLock_;
}

void AudioNodeManager::addNode(const std::shared_ptr<AudioNode> &node) {
  Locker lock(getGraphLock());
  nodes_.emplace_back(node);
}

void AudioNodeManager::settlePendingConnections() {
  for (auto it = audioNodesToConnect_.begin(); it != audioNodesToConnect_.end();
       ++it) {
    std::shared_ptr<AudioNode> from = std::get<0>(*it);
    std::shared_ptr<AudioNode> to = std::get<1>(*it);
    ConnectionType type = std::get<2>(*it);

    assert(from != nullptr);
    assert(to != nullptr);

    if (type == ConnectionType::CONNECT) {
      from->connectNode(to);
    } else {
      from->disconnectNode(to, true);
    }
  }

  audioNodesToConnect_.clear();
}

void AudioNodeManager::prepareNodesForDestruction() {
  auto it = nodes_.begin();

  while (it != nodes_.end()) {
    if (it->use_count() == 1) {
      assert(it->get()->inputNodes_.size() == 0);
      it->get()->cleanup(false);
      it = nodes_.erase(it);
    } else {
      ++it;
    }
  }
}

void AudioNodeManager::cleanup() {
  Locker lock(getGraphLock());

  for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
    it->get()->cleanup(false);
  }

  nodes_.clear();
  audioNodesToConnect_.clear();
}

} // namespace audioapi
