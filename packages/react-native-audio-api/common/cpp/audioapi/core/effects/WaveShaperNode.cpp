#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>
#include <audioapi/utils/AudioBuffer.h>

#include <algorithm>
#include <memory>
#include <string>

namespace audioapi {

WaveShaperNode::WaveShaperNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const WaveShaperOptions &options)
    : AudioNode(context, options), oversample_(options.oversample) {

  waveShapers_.reserve(6);
  for (size_t i = 0; i < channelCount_; i++) {
    waveShapers_.emplace_back(std::make_unique<WaveShaper>(nullptr));
  }
  setCurve(options.curve);
  isInitialized_ = true;
}

OverSampleType WaveShaperNode::getOversample() const {
  return oversample_.load(std::memory_order_acquire);
}

void WaveShaperNode::setOversample(OverSampleType type) {
  std::scoped_lock<std::mutex> lock(mutex_);
  oversample_.store(type, std::memory_order_release);

  for (int i = 0; i < waveShapers_.size(); i++) {
    waveShapers_[i]->setOversample(type);
  }
}

std::shared_ptr<AudioArrayBuffer> WaveShaperNode::getCurve() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return curve_;
}

void WaveShaperNode::setCurve(const std::shared_ptr<AudioArrayBuffer> &curve) {
  std::scoped_lock<std::mutex> lock(mutex_);
  curve_ = curve;

  for (int i = 0; i < waveShapers_.size(); i++) {
    waveShapers_[i]->setCurve(curve);
  }
}

std::shared_ptr<AudioBuffer> WaveShaperNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  if (!isInitialized_) {
    return processingBuffer;
  }

  std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);

  if (!lock.owns_lock()) {
    return processingBuffer;
  }

  if (curve_ == nullptr) {
    return processingBuffer;
  }

  for (size_t channel = 0; channel < processingBuffer->getNumberOfChannels(); channel++) {
    auto channelData = processingBuffer->getChannel(channel);

    waveShapers_[channel]->process(*channelData, framesToProcess);
  }

  return processingBuffer;
}

} // namespace audioapi
