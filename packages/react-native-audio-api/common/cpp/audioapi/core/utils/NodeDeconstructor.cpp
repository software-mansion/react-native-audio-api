#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/Locker.h>
#include <audioapi/core/utils/NodeDeconstructor.h>

namespace audioapi {

NodeDeconstructor::NodeDeconstructor() : isExiting_(false) {
  thread_ = std::thread(&NodeDeconstructor::process, this);
}

NodeDeconstructor::~NodeDeconstructor() {
  stop();
}

std::mutex &NodeDeconstructor::getLock() {
  return lock_;
}

void NodeDeconstructor::addNodeForDeconstruction(
    const std::shared_ptr<AudioNode> &node) {
  nodesForDeconstruction_.emplace_back(node);
}

void NodeDeconstructor::wakeDeconstructor() {
  Locker lock(lock_);
  condition_.notify_one();
}

void NodeDeconstructor::stop() {
  {
    Locker lock(lock_);
    isExiting_ = true;
  }

  condition_.notify_one();
  if (thread_.joinable()) {
    thread_.join();
  }
}

void NodeDeconstructor::process() {
  std::unique_lock<std::mutex> lock(lock_);
  while (!isExiting_) {
    condition_.wait(lock, [this] {
      return isExiting_ || !nodesForDeconstruction_.empty();
    });

    if (isExiting_)
      break;

    if (!isExiting_ && !nodesForDeconstruction_.empty()) {
      nodesForDeconstruction_.clear();
    }
  }
}

} // namespace audioapi
