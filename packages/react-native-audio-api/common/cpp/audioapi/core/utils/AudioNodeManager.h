#pragma once

#include <audioapi/core/utils/AudioNodeDestructor.h>

#include <memory>
#include <mutex>
#include <tuple>
#include <vector>
#include <unordered_set>

namespace audioapi {

class AudioNode;
class AudioScheduledSourceNode;
class AudioParam;

class AudioNodeManager {
 public:
  enum class ConnectionType { CONNECT, DISCONNECT };
  AudioNodeManager() = default;
  ~AudioNodeManager();

  std::mutex &getGraphLock();

  void preProcessGraph();

  void addPendingNodeConnection(
      const std::shared_ptr<AudioNode> &from,
      const std::shared_ptr<AudioNode> &to,
      ConnectionType type);

  void addPendingParamConnection(
      const std::shared_ptr<AudioNode> &from,
      const std::shared_ptr<AudioParam> &to,
    ConnectionType type);

  void addProcessingNode(const std::shared_ptr<AudioNode> &node);
  void addSourceNode(const std::shared_ptr<AudioScheduledSourceNode> &node);
  void addAudioParam(const std::shared_ptr<AudioParam> &param);

  void cleanup();

 private:
  std::mutex graphLock_;
  AudioNodeDestructor nodeDeconstructor_;

  // all nodes created in the context
  std::unordered_set<std::shared_ptr<AudioScheduledSourceNode>> sourceNodes_;
  std::unordered_set<std::shared_ptr<AudioNode>> processingNodes_;
  std::unordered_set<std::shared_ptr<AudioParam>> audioParams_;

  // connections to be settled
  std::vector<std::tuple<
      std::shared_ptr<AudioNode>,
      std::shared_ptr<AudioNode>,
      ConnectionType>>
      audioNodesToConnect_;

  std::vector<std::tuple<
      std::shared_ptr<AudioNode>,
      std::shared_ptr<AudioParam>,
      ConnectionType>>
      audioParamToConnect_;

  void settlePendingConnections();
  void cleanupNode(const std::shared_ptr<AudioNode> &node);
  void prepareNodesForDestruction();
};

} // namespace audioapi
