#include <audioapi/core/inputs/RecorderInputEvents.h>

#include <audioapi/core/inputs/AudioRecorder.h>

#include <memory>
#include <string>

namespace audioapi {

void RecorderInputEvents::formatChanged() const {
  if (auto recorder = recorder_.lock()) {
    recorder->onInputFormatChanged(openedGeneration_);
  }
}

void RecorderInputEvents::lost() const {
  if (auto recorder = recorder_.lock()) {
    recorder->rerouteInput(openedGeneration_);
  }
}

void RecorderInputEvents::failed(const std::string &reason) const {
  if (auto recorder = recorder_.lock()) {
    recorder->onInputFailed(openedGeneration_, reason);
  }
}

} // namespace audioapi
