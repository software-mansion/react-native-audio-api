#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/AudioNodeDestructor.h>
#include <audioapi/core/utils/Locker.h>

namespace audioapi {

AudioNodeDestructor::AudioNodeDestructor()
    : thread_(std::thread(&AudioNodeDestructor::process, this)),
      isExiting_(false) {}

AudioNodeDestructor::~AudioNodeDestructor() {
  isExiting_ = true;

  cv_.notify_one(); // call process for the last time
  if (thread_.joinable()) {
    thread_.join();
  }
}

void AudioNodeDestructor::tryCallWithLock(
    const std::function<void()> &callback) {
  if (auto lock = Locker::tryLock(mutex_)) {
    callback();
  }
}

void AudioNodeDestructor::addNodeForDeconstruction(
    const std::shared_ptr<AudioNode> &node) {
  // NOTE: this method must be called within `tryCallWithLock`
  nodesForDeconstruction_.emplace_back(node);
}

void AudioNodeDestructor::notify() {
  cv_.notify_one();
}

void AudioNodeDestructor::process() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (!isExiting_) {
    cv_.wait(lock, [this] {
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
