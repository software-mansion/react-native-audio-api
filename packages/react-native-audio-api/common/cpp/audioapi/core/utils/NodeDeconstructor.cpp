#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/core/utils/NodeDeconstructor.h>

namespace audioapi {

NodeDeconstructor::NodeDeconstructor() {
  printf("NodeDeconstructor::NodeDeconstructor()\n");
  thread_ = std::thread(&NodeDeconstructor::process, this);
  thread_.detach();
}

NodeDeconstructor::~NodeDeconstructor() {
  printf("NodeDeconstructor::~NodeDeconstructor()\n");
  Locker lock(getLock());
  isExiting_ = true;

  condition_.notify_all();

  if (thread_.joinable()) {
    thread_.join();
  }
}

std::mutex &NodeDeconstructor::getLock() {
  return lock_;
}

void NodeDeconstructor::addNodeForDeconstruction(
    const std::shared_ptr<AudioNode> &node) {
  nodesForDeconstruction_.emplace_back(node);
}

void NodeDeconstructor::wakeDeconstructor() {
  condition_.notify_one();
}

void NodeDeconstructor::process() {
  printf("NodeDeconstructor::process() entry\n");

  while (!isExiting_) {
    std::unique_lock<std::mutex> lock(lock_);
    condition_.wait(
        lock, [this] { return nodesForDeconstruction_.size() > 0; });

    printf("NodeDeconstructor::process()\n");
    nodesForDeconstruction_.clear();
  }
}

} // namespace audioapi
