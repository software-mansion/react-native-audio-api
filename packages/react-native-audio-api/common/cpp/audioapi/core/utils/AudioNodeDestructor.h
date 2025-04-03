#pragma once

#include <condition_variable>
#include <mutex>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>

namespace audioapi {

class AudioNode;

class AudioNodeDestructor {
 public:
  AudioNodeDestructor();
  ~AudioNodeDestructor();

  void callWithLock(const std::function<void()> &callback);
  void addNodeForDeconstruction(const std::shared_ptr<AudioNode> &node);
  void notify();

 private:
  mutable std::mutex mutex_;
  std::thread thread_;
  std::condition_variable cv_;
  std::vector<std::shared_ptr<AudioNode>> nodesForDeconstruction_;

  std::atomic<bool> isExiting_;

  void process();
};
} // namespace audioapi
