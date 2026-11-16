#include <audioapi/core/inputs/RecorderInputGate.h>

#include <audioapi/core/inputs/AudioRecorder.h>

#include <thread>

namespace audioapi {

// deliver() increments callbacksInFlight_ and then reads armed_; disarmAndWait() clears armed_ and
// then reads callbacksInFlight_. Each side stores one variable and loads the other, and only
// seq_cst rules out both loads seeing the old values: with acquire/release, a store can still sit
// in its core's store buffer when the other core loads, so a callback could run while the
// recorder changes its state.
void RecorderInputGate::deliver(const float *const *channels, int channelCount, int numFrames) {
  callbacksInFlight_.fetch_add(1, std::memory_order_seq_cst);

  if (armed_.load(std::memory_order_seq_cst)) {
    recorder_->onInputFrames(channels, channelCount, numFrames);
  }

  callbacksInFlight_.fetch_sub(1, std::memory_order_release);
}

void RecorderInputGate::arm() {
  armed_.store(true, std::memory_order_seq_cst);
}

void RecorderInputGate::disarmAndWait() {
  armed_.store(false, std::memory_order_seq_cst);

  while (callbacksInFlight_.load(std::memory_order_seq_cst) != 0) {
    std::this_thread::yield();
  }
}

} // namespace audioapi
