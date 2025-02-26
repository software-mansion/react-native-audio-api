#include <cassert>

#include "StretcherNode.h"
#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"

namespace audioapi {

StretcherNode::StretcherNode(BaseAudioContext *context) : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::EXPLICIT;
  rate_ = std::make_shared<AudioParam>(1.0, 0.0, 3.0);

  stretch_ =
      std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>();
  stretch_->presetDefault(channelCount_, context->getSampleRate());
  playbackRateBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE * 3, channelCount_, context_->getSampleRate());

  isInitialized_ = true;
}

std::string StretcherNode::getTimeStretch() const {
  return toString(timeStretch_);
}

std::shared_ptr<AudioParam> StretcherNode::getRate() const { return rate_; }

void StretcherNode::setTimeStretch(const std::string &timeStretchType) {
  timeStretch_ = fromString(timeStretchType);
}

void StretcherNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {}

std::shared_ptr<AudioBus> StretcherNode::processAudio(
        std::shared_ptr<AudioBus> outputBus,
        int framesToProcess) {

    if (!isInitialized_) {
        return outputBus;
    }

    if (isAlreadyProcessed()) {
        return audioBus_;
    }

    auto time = context_->getCurrentTime();
    auto rate = rate_->getValueAtTime(time);
    auto framesToStretch = static_cast<int>(rate * static_cast<float>(framesToProcess));

    playbackRateBus_->zero();

    while (framesToStretch > 0) {
        // Process inputs and return the bus with the most channels.
        auto processingBus = processInputs(outputBus, framesToProcess);

        // Apply channel count mode.
        processingBus = applyChannelCountMode(processingBus);

        // Mix all input buses into the processing bus.
        mixInputsBuses(processingBus);

        assert(processingBus != nullptr);

        playbackRateBus_->copy(processingBus.get(), 0, 0, 0);
    }

    processNode(playbackRateBus_, framesToProcess);

    return audioBus_;
}

} // namespace audioapi
