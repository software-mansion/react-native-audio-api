#include "StretcherNode.h"
#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"

namespace audioapi {

StretcherNode::StretcherNode(BaseAudioContext *context) : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::EXPLICIT;

  stretch_ =
      std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>();
  stretch_->presetDefault(channelCount_, context->getSampleRate());
  playbackRateBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE * 2, 1, context_->getSampleRate());

  isInitialized_ = true;
}

std::string StretcherNode::getTimeStretch() const {
  return toString(timeStretch_);
}

void StretcherNode::setTimeStretch(const std::string &timeStretchType) {
  timeStretch_ = fromString(timeStretchType);
}

void StretcherNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {}

} // namespace audioapi
