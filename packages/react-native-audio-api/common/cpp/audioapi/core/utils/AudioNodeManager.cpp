#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/AudioNodeManager.h>
#include <audioapi/core/utils/Locker.h>

namespace audioapi {

AudioNodeManager::~AudioNodeManager() {
  cleanup();
}

void AudioNodeManager::addPendingNodeConnection(
    const std::shared_ptr<AudioNode> &from,
    const std::shared_ptr<AudioNode> &to,
    ConnectionType type) {
  Locker lock(getGraphLock());

  audioNodesToConnect_.emplace_back(from, to, type);
}

void AudioNodeManager::addPendingParamConnection(
    const std::shared_ptr<AudioNode> &from,
    const std::shared_ptr<AudioParam> &to,
    ConnectionType type) {
  Locker lock(getGraphLock());

  audioParamToConnect_.emplace_back(from, to, type);
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

void AudioNodeManager::addProcessingNode(
    const std::shared_ptr<AudioNode> &node) {
  Locker lock(getGraphLock());
  processingNodes_.insert(node);
}

void AudioNodeManager::addSourceNode(
    const std::shared_ptr<AudioScheduledSourceNode> &node) {
  Locker lock(getGraphLock());
  sourceNodes_.insert(node);
}

void AudioNodeManager::addAudioParam(const std::shared_ptr<AudioParam> &param) {
  Locker lock(getGraphLock());
  audioParams_.insert(param);
}

void AudioNodeManager::settlePendingConnections() {
  for (auto it = audioNodesToConnect_.begin(), end = audioNodesToConnect_.end();
       it != end;
       ++it) {
    std::shared_ptr<AudioNode> from = std::get<0>(*it);
    std::shared_ptr<AudioNode> to = std::get<1>(*it);
    ConnectionType type = std::get<2>(*it);

    assert(from != nullptr);
    assert(to != nullptr);

    if (type == ConnectionType::CONNECT) {
      from->connectNode(to);
    } else {
      from->disconnectNode(to);
    }
  }

  audioNodesToConnect_.clear();

  for (auto it = audioParamToConnect_.begin(), end = audioParamToConnect_.end();
       it != end;
       ++it) {
    std::shared_ptr<AudioNode> from = std::get<0>(*it);
    std::shared_ptr<AudioParam> to = std::get<1>(*it);
    ConnectionType type = std::get<2>(*it);

    assert(from != nullptr);
    assert(to != nullptr);

    if (type == ConnectionType::CONNECT) {
      from->connectParam(to);
    } else {
      from->disconnectParam(to);
    }
  }

  audioParamToConnect_.clear();
}

void AudioNodeManager::cleanupNode(const std::shared_ptr<AudioNode> &node) {
  nodeDeconstructor_.addNodeForDeconstruction(node);
  node.get()->cleanup();
}

void AudioNodeManager::prepareNodesForDestruction() {
  nodeDeconstructor_.tryCallWithLock([this]() {
    auto sNodesIt = sourceNodes_.begin();

    while (sNodesIt != sourceNodes_.end()) {
      // we don't want to destroy nodes that are still playing or will be
      // playing
      if (sNodesIt->use_count() == 1 &&
          (sNodesIt->get()->isUnscheduled() || sNodesIt->get()->isFinished())) {
        cleanupNode(*sNodesIt);
        sNodesIt = sourceNodes_.erase(sNodesIt);
      } else {
        ++sNodesIt;
      }
    }

    auto pNodesIt = processingNodes_.begin();

    while (pNodesIt != processingNodes_.end()) {
      if (pNodesIt->use_count() == 1) {
        cleanupNode(*pNodesIt);
        pNodesIt = processingNodes_.erase(pNodesIt);
      } else {
        ++pNodesIt;
      }
    }
  });
  nodeDeconstructor_.notify();
}

void AudioNodeManager::cleanup() {
  Locker lock(getGraphLock());

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
}

} // namespace audioapi
