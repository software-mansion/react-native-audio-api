#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

WaveShaperNode::WaveShaperNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const WaveShaperOptions &options)
    : AudioNode(context, options), oversample_(options.oversample) {

  waveShapers_.reserve(6);
  for (int i = 0; i < channelCount_; i++) {
    waveShapers_.emplace_back(std::make_unique<WaveShaper>(nullptr, context->getSampleRate()));
  }
  setCurve(options.curve);
  isInitialized_.store(true, std::memory_order_release);
}

void WaveShaperNode::setOversample(OverSampleType type) {
  oversample_ = type;

  for (int i = 0; i < waveShapers_.size(); i++) {
    waveShapers_[i]->setOversample(type);
  }
}

void WaveShaperNode::setCurve(const std::shared_ptr<AudioArray> &curve) {
  curve_ = curve;

  for (int i = 0; i < waveShapers_.size(); i++) {
    waveShapers_[i]->setCurve(curve);
  }
}

std::shared_ptr<DSPAudioBuffer> WaveShaperNode::processNode(
    const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (curve_ == nullptr) {
    return processingBuffer;
  }

  for (size_t channel = 0; channel < processingBuffer->getNumberOfChannels(); channel++) {
    auto *channelData = processingBuffer->getChannel(channel);

    waveShapers_[channel]->process(*channelData, framesToProcess);
  }

  return processingBuffer;
}

} // namespace audioapi
