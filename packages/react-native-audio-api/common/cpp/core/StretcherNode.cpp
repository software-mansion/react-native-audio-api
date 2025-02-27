#include <cassert>

#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"
#include "StretcherNode.h"

namespace audioapi {

StretcherNode::StretcherNode(BaseAudioContext *context) : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::EXPLICIT;
  rate_ = std::make_shared<AudioParam>(1.0, 0.0, 3.0);
  semitones_ = std::make_shared<AudioParam>(0.0, -12.0, 12.0);

  stretch_ =
      std::make_shared<signalsmith::stretch::SignalsmithStretch<float>>();
  stretch_->presetDefault(channelCount_, context->getSampleRate());
  playbackRateBus_ = std::make_shared<AudioBus>(
      RENDER_QUANTUM_SIZE * 3, channelCount_, context_->getSampleRate());

  isInitialized_ = true;
}

std::shared_ptr<AudioParam> StretcherNode::getRateParam() const {
  return rate_;
}

std::shared_ptr<AudioParam> StretcherNode::getSemitonesParam() const {
  return semitones_;
}

void StretcherNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  auto time = context_->getCurrentTime();
  auto semitones = semitones_->getValueAtTime(time);

  stretch_->setTransposeSemitones(semitones);
  stretch_->process(
      playbackRateBus_.get()[0],
      framesNeededToStretch_,
      audioBus_.get()[0],
      framesToProcess);
}

std::shared_ptr<AudioBus> StretcherNode::processAudio(
    std::shared_ptr<AudioBus> outputBus,
    int framesToProcess,
    bool checkIsAlreadyProcessed) {
  if (!isInitialized_) {
    return outputBus;
  }

  if (isAlreadyProcessed()) {
    return audioBus_;
  }

  auto time = context_->getCurrentTime();

  auto rate = rate_->getValueAtTime(time);
  framesNeededToStretch_ =
      static_cast<int>(rate * static_cast<float>(framesToProcess));

  playbackRateBus_->zero();
  auto writeIndex = 0;
  auto framesNeededToStretch = framesNeededToStretch_;

  // Collecting frames needed to stretch
  while (framesNeededToStretch > 0) {
    auto framesToCopy = std::min(framesNeededToStretch, framesToProcess);

    // Process inputs and return the bus with the most channels. We must not
    // check if the node has already been processed, cause we need to process it
    // multiple times in this case.
    auto processingBus = processInputs(outputBus, framesToCopy, false);

    // Apply channel count mode.
    processingBus = applyChannelCountMode(processingBus);

    // Mix all input buses into the processing bus.
    mixInputsBuses(processingBus);

    assert(processingBus != nullptr);

    playbackRateBus_->copy(processingBus.get(), 0, writeIndex, framesToCopy);

    writeIndex += framesToCopy;
    framesNeededToStretch -= framesToCopy;
  }

  processNode(audioBus_, framesToProcess);

  return audioBus_;
}

} // namespace audioapi
