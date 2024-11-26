#pragma once

#include <mutex>
#include <tuple>
#include <memory>
#include <vector>

namespace audioapi {

class AudioNode;

class AudioNodeManager {
  public:
    enum class ConnectionType { CONNECT, DISCONNECT };
    AudioNodeManager();
    ~AudioNodeManager();

    void preProcessGraph();
    void addPendingConnection(const std::shared_ptr<AudioNode> &from, const std::shared_ptr<AudioNode> &to, ConnectionType type);

    void addSourceNode(const std::shared_ptr<AudioNode> &node);

    std::mutex& getGraphLock();

  private:
    std::mutex graphLock_;

    std::vector<std::shared_ptr<AudioNode>> sourceNodes_;
    std::vector<std::tuple<std::shared_ptr<AudioNode>, std::shared_ptr<AudioNode>, ConnectionType>> audioNodesToConnect_;

    void settlePendingConnections();
    void removeFinishedSourceNodes();
};

} // namespace audioapi
