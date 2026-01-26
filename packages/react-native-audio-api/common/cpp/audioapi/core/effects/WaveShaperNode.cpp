#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/HostObjects/utils/NodeOptions.h>

#include <algorithm>
#include <memory>
#include <string>

namespace audioapi {

WaveShaperNode::WaveShaperNode(std::shared_ptr<BaseAudioContext> context, const WaveShaperOptions &options)
    : AudioNode(context, options), oversample_(options.oversample) {

  waveShapers_.reserve(6);
  for (int i = 0; i < channelCount_; i++) {
    waveShapers_.emplace_back(std::make_unique<WaveShaper>(nullptr));
  }
  setCurve(options.curve);
  // to change after graph processing improvement - should be max
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
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

std::shared_ptr<AudioArray> WaveShaperNode::getCurve() const {
  std::scoped_lock<std::mutex> lock(mutex_);
  return curve_;
}

void WaveShaperNode::setCurve(const std::shared_ptr<AudioArray> &curve) {
  std::scoped_lock<std::mutex> lock(mutex_);
  curve_ = curve;

  for (int i = 0; i < waveShapers_.size(); i++) {
    waveShapers_[i]->setCurve(curve);
  }
}

std::shared_ptr<AudioBus> WaveShaperNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  if (!isInitialized_) {
    return processingBus;
  }

  std::unique_lock<std::mutex> lock(mutex_, std::try_to_lock);

  if (!lock.owns_lock()) {
    return processingBus;
  }

  if (curve_ == nullptr) {
    return processingBus;
  }

  for (int channel = 0; channel < processingBus->getNumberOfChannels(); channel++) {
    auto channelData = processingBus->getSharedChannel(channel);

    waveShapers_[channel]->process(channelData, framesToProcess);
  }

  return processingBus;
}

} // namespace audioapi
