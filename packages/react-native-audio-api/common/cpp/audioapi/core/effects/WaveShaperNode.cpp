#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/WaveShaperNode.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/types/NodeOptions.h>

#include <algorithm>
#include <memory>
#include <utility>

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
}

void WaveShaperNode::setOversample(
    std::unique_ptr<OversampleUpdate> update,
    utils::Disposer<DISPOSER_PAYLOAD_SIZE> &disposer) {
  oversample_ = update->type;

  const size_t n = std::min(waveShapers_.size(), update->pairs.size());
  for (size_t i = 0; i < n; i++) {
    auto &[up, down] = update->pairs[i];
    waveShapers_[i]->setOversample(update->type, std::move(up), std::move(down), disposer);
  }
  // Ship the wrapper itself off the audio thread. Only the 8-byte
  // unique_ptr crosses the disposer; ~OversampleUpdate (and its now-empty
  // vector of moved-from unique_ptrs) runs on the worker thread.
  disposer.dispose(std::move(update));
}

void WaveShaperNode::setCurve(const std::shared_ptr<AudioArray> &curve) {
  curve_ = curve;

  for (int i = 0; i < waveShapers_.size(); i++) {
    waveShapers_[i]->setCurve(curve);
  }
}

void WaveShaperNode::processNode(int framesToProcess) {
  if (curve_ == nullptr) {
    return;
  }

  for (size_t channel = 0; channel < audioBuffer_->getNumberOfChannels(); channel++) {
    auto *channelData = audioBuffer_->getChannel(channel);

    waveShapers_[channel]->process(*channelData, framesToProcess);
  }
}

} // namespace audioapi
