#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>

namespace audioapi {
AudioBufferBaseSourceNode::AudioBufferBaseSourceNode(BaseAudioContext *context)
    : AudioScheduledSourceNode(context) {
  onPositionChangedInterval_ = static_cast<int>(context->getSampleRate() * 0.1);
}

void AudioBufferBaseSourceNode::setOnPositionChangedCallbackId(
    uint64_t callbackId) {
  onPositionChangedCallbackId_ = callbackId;
}

void AudioBufferBaseSourceNode::setOnPositionChangedInterval(int interval) {
  onPositionChangedInterval_ = static_cast<int>(
      context_->getSampleRate() * static_cast<float>(interval) / 1000);
}

void AudioBufferBaseSourceNode::sendOnPositionChangedEvent() {
  if (onPositionChangedTime_ > onPositionChangedInterval_) {
    std::unordered_map<std::string, EventValue> body = {
        {"value", getCurrentPosition()}};

    context_->audioEventHandlerRegistry_->invokeHandlerWithEventBody(
        "positionChanged", onPositionChangedCallbackId_, body);

    onPositionChangedTime_ = 0;
  }

  onPositionChangedTime_ += RENDER_QUANTUM_SIZE;
}

} // namespace audioapi
