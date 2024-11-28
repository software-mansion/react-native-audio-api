
#include "AudioNodeManager.h"
#include "AudioNode.h"
#include "Locker.h"

namespace audioapi {

AudioNodeManager::AudioNodeManager() {}

AudioNodeManager::~AudioNodeManager() {
  audioNodesToConnect_.clear();
  sourceNodes_.clear();
}

void AudioNodeManager::addPendingConnection(
    const std::shared_ptr<AudioNode> &from,
    const std::shared_ptr<AudioNode> &to,
    ConnectionType type) {
  Locker lock(getGraphLock());

  audioNodesToConnect_.push_back(std::make_tuple(from, to, type));
}

void AudioNodeManager::addSourceNode(const std::shared_ptr<AudioNode> &node) {
  Locker lock(getGraphLock());

  sourceNodes_.push_back(node);
}

void AudioNodeManager::preProcessGraph() {
  if (!Locker::tryLock(getGraphLock())) {
    return;
  }

  settlePendingConnections();
  removeFinishedSourceNodes();
}

std::mutex &AudioNodeManager::getGraphLock() {
  return graphLock_;
}

void AudioNodeManager::settlePendingConnections() {
  for (auto &connection : audioNodesToConnect_) {
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

void AudioNodeManager::removeFinishedSourceNodes() {
  for (auto it = sourceNodes_.begin(); it != sourceNodes_.end();) {
    auto currentNode = it->get();
    // Release the source node if use count is equal to 1 (this vector)
    if (!currentNode->isEnabled() && it->use_count() == 1) {
      for (auto &outputNode : currentNode->outputNodes_) {
        currentNode->disconnectNode(outputNode);
      }

      it = sourceNodes_.erase(it);
    } else {
      ++it;
    }
  }
}

} // namespace audioapi
