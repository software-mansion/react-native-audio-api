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
    void postProcessGraph();
    void addPendingConnection(std::shared_ptr<AudioNode> from, std::shared_ptr<AudioNode> to, ConnectionType type);

    void setNodeToDelete(std::shared_ptr<AudioNode> node);

    std::mutex& getGraphLock();

  private:
    std::mutex graphLock_;

    std::vector<std::tuple<std::shared_ptr<AudioNode>, std::shared_ptr<AudioNode>, ConnectionType>> audioNodesToConnect_;
    std::vector<std::shared_ptr<AudioNode>> audioNodesToDelete_;

    void settlePendingConnections();
    void settlePendingDeletions();
};

} // namespace audioapi
