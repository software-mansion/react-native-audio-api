#include <audioapi/core/AudioNode.h>
#include <audioapi/core/utils/AudioNodeDestructor.h>
#include <audioapi/core/utils/Locker.h>

namespace audioapi {

AudioNodeDestructor::AudioNodeDestructor() {
  isExiting_.store(false, std::memory_order_release);
  auto [sender, receiver] = channels::spsc::channel<
      std::shared_ptr<AudioNode>,
      channels::spsc::OverflowStrategy::WAIT_ON_FULL,
      channels::spsc::WaitStrategy::ATOMIC_WAIT>(kChannelCapacity);
  sender_ = std::move(sender);
  workerHandle_ =
      std::thread(&AudioNodeDestructor::process, this, std::move(receiver));
}

AudioNodeDestructor::~AudioNodeDestructor() {
  isExiting_.store(true, std::memory_order_release);

  // We need to send a nullptr to unblock the receiver
  sender_.send(nullptr);
  if (workerHandle_.joinable()) {
    workerHandle_.join();
  }
}

bool AudioNodeDestructor::tryAddNodeForDeconstruction(
    std::shared_ptr<AudioNode> &&node) {
  return sender_.try_send(std::move(node)) ==
      channels::spsc::ResponseStatus::SUCCESS;
}

void AudioNodeDestructor::process(
    channels::spsc::Receiver<
        std::shared_ptr<AudioNode>,
        channels::spsc::OverflowStrategy::WAIT_ON_FULL,
        channels::spsc::WaitStrategy::ATOMIC_WAIT> &&receiver) {
  auto rcv = std::move(receiver);
  while (!isExiting_.load(std::memory_order_acquire)) {
    rcv.receive();
  }
}

} // namespace audioapi
