#pragma once

#include <audioapi/core/utils/AudioNodeDestructor.h>

#include <memory>
#include <mutex>
#include <tuple>
#include <vector>
#include <unordered_set>

namespace audioapi {

class AudioNode;

class AudioNodeManager {
 public:
  enum class ConnectionType { CONNECT, DISCONNECT };
  AudioNodeManager() = default;
  ~AudioNodeManager();

  std::mutex &getGraphLock();

  void preProcessGraph();

  void addPendingConnection(
      const std::shared_ptr<AudioNode> &from,
      const std::shared_ptr<AudioNode> &to,
      ConnectionType type);

  void addNode(const std::shared_ptr<AudioNode> &node);

  void cleanup();

 private:
  std::mutex graphLock_;
  AudioNodeDestructor nodeDeconstructor_;

  // all nodes created in the context
  std::unordered_set<std::shared_ptr<AudioNode>> nodes_;

  // connections to be settled
  std::vector<std::tuple<
      std::shared_ptr<AudioNode>,
      std::shared_ptr<AudioNode>,
      ConnectionType>>
      audioNodesToConnect_;

  void settlePendingConnections();
  void prepareNodesForDestruction();
};

} // namespace audioapi
