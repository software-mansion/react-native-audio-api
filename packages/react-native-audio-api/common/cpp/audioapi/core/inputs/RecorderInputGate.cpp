#include <audioapi/core/inputs/RecorderInputGate.h>

#include <audioapi/core/inputs/AudioRecorder.h>

#include <thread>

namespace audioapi {

// Each side stores one atomic, then loads the other; seq_cst keeps both loads from missing the
// other side's store. Increment before reading armed_, so a drain can never miss this pass.
RecorderInputGate::Pass::Pass(RecorderInputGate &gate) : gate_(gate) {
  gate_.callbacksInFlight_.fetch_add(1, std::memory_order_seq_cst);
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  isAdmitted_ = gate_.armed_.load(std::memory_order_seq_cst);
}

RecorderInputGate::Pass::~Pass() {
  gate_.callbacksInFlight_.fetch_sub(1, std::memory_order_release);
}

void RecorderInputGate::Pass::deliver(const float *const *channels, int channelCount, int numFrames)
    const {
  if (isAdmitted_) {
    gate_.recorder_->onInputFrames(channels, channelCount, numFrames);
  }
}

RecorderInputGate::Pass RecorderInputGate::enter() {
  return Pass(*this);
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
