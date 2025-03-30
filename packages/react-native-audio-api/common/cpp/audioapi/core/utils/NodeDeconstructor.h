#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>

namespace audioapi {

class AudioNode;

class NodeDeconstructor {
 public:
  NodeDeconstructor();
  ~NodeDeconstructor();

  std::mutex &getLock();

  void addNodeForDeconstruction(const std::shared_ptr<AudioNode> &node);
  void wakeDeconstructor();

 private:
  std::mutex lock_;
  std::thread thread_;
  std::condition_variable condition_;
  std::vector<std::shared_ptr<AudioNode>> nodesForDeconstruction_;

  bool isExiting_ = false;

  void process();
};

} // namespace audioapi
