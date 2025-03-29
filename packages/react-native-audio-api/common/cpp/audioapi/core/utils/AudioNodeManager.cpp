#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/Locker.h>

namespace audioapi {

AudioNodeManager::~AudioNodeManager() {
  cleanup();
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

  nodes_.push_back(std::move(node));
}

void AudioNodeManager::settlePendingConnections() {
  for (auto it = audioNodesToConnect_.begin(); it != audioNodesToConnect_.end(); ++it) {
    std::shared_ptr<AudioNode> from = std::get<0>(*it);
    std::shared_ptr<AudioNode> to = std::get<1>(*it);
    ConnectionType type = std::get<2>(*it);

    if (!to || !from) {
      continue;
    }

    if (type == ConnectionType::CONNECT) {
      from->connectNode(to);
    } else {
      from->disconnectNode(to, true);
    }
  }

  audioNodesToConnect_.clear();
}

void AudioNodeManager::prepareNodesForDestruction() {
  nodes_.erase(
    std::remove_if(
      nodes_.begin(),
      nodes_.end(),
      [](std::shared_ptr<AudioNode> const &node) {
        return node == nullptr || node.use_count() == 1;
      }
    ),
    nodes_.end()
  );
}

void AudioNodeManager::cleanup() {
  Locker lock(getGraphLock());

  for (auto it = nodes_.begin(); it != nodes_.end(); ++it) {
    it->get()->cleanup();
  }

  nodes_.clear();
}

} // namespace audioapi
