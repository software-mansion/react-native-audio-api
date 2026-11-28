#include <audioapi/core/inputs/RecorderInputGate.h>

#include <thread>

namespace audioapi {

// Each side stores one atomic, then loads the other; seq_cst keeps both loads from missing the
// other side's store. Increment before reading armed_, so a drain can never miss this pass.
RecorderInputGate::Pass::Pass(RecorderInputGate &gate) : gate(gate) {
  gate.callbacksInFlight_.fetch_add(1, std::memory_order_seq_cst);
  // NOLINTNEXTLINE(cppcoreguidelines-prefer-member-initializer)
  isAdmitted = gate.armed_.load(std::memory_order_seq_cst);
}

RecorderInputGate::Pass::~Pass() {
  gate.callbacksInFlight_.fetch_sub(1, std::memory_order_release);
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
