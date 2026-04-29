#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/dsp/WaveShaper.h>

#include <cstring>
#include <memory>
#include <utility>

namespace audioapi {

WaveShaper::WaveShaper(const std::shared_ptr<AudioArray> &curve, float sampleRate)
    : curve_(curve), sampleRate_(sampleRate) {
  tempBuffer2x_ = std::make_shared<DSPAudioArray>(RENDER_QUANTUM_SIZE * 2);
  tempBuffer2x_->zero();
  tempBuffer4x_ = std::make_shared<DSPAudioArray>(RENDER_QUANTUM_SIZE * 4);
  tempBuffer4x_->zero();
}

std::pair<SingleChannelResamplerPtr, SingleChannelResamplerPtr> WaveShaper::makeResamplers(
    OverSampleType type,
    float sampleRate) {
  if (type == OverSampleType::OVERSAMPLE_2X) {
    return {
        std::make_unique<r8b::SingleChannelResampler>(
            sampleRate, sampleRate * 2, RENDER_QUANTUM_SIZE),
        std::make_unique<r8b::SingleChannelResampler>(
            sampleRate * 2, sampleRate, RENDER_QUANTUM_SIZE * 2)};
  }
  if (type == OverSampleType::OVERSAMPLE_4X) {
    return {
        std::make_unique<r8b::SingleChannelResampler>(
            sampleRate, sampleRate * 4, RENDER_QUANTUM_SIZE * 2),
        std::make_unique<r8b::SingleChannelResampler>(
            sampleRate * 4, sampleRate, RENDER_QUANTUM_SIZE * 4)};
  }
  return {nullptr, nullptr};
}

void WaveShaper::setCurve(const std::shared_ptr<AudioArray> &curve) {
  curve_ = curve;
}

void WaveShaper::setOversample(
    OverSampleType type,
    SingleChannelResamplerPtr upSampler,
    SingleChannelResamplerPtr downSampler,
    utils::Disposer<DISPOSER_PAYLOAD_SIZE> &disposer) {
  oversample_ = type;
  // Ship previous resamplers off the audio thread; only the 8-byte
  // unique_ptr crosses the disposer's SPSC, the actual ~CDSPResampler24
  // chain runs on the disposer worker thread.
  if (upSampler_) {
    disposer.dispose(std::move(upSampler_));
  }
  if (downSampler_) {
    disposer.dispose(std::move(downSampler_));
  }
  upSampler_ = std::move(upSampler);
  downSampler_ = std::move(downSampler);
}

void WaveShaper::process(DSPAudioArray &channelData, int framesToProcess) {
  if (curve_ == nullptr) {
    return;
  }

  switch (oversample_) {
    case OverSampleType::OVERSAMPLE_2X:
    case OverSampleType::OVERSAMPLE_4X:
      processResampled(channelData, framesToProcess);
      break;
    case OverSampleType::OVERSAMPLE_NONE:
    default:
      processNone(channelData, framesToProcess);
      break;
  }
}

// based on https://webaudio.github.io/web-audio-api/#WaveShaperNode
void WaveShaper::processNone(DSPAudioArray &channelData, int framesToProcess) {
  auto curveSize = curve_->getSize();

  for (int i = 0; i < framesToProcess; i++) {
    float v = (static_cast<float>(curveSize) - 1) * 0.5f * (channelData[i] + 1.0f);

    if (v <= 0) {
      channelData[i] = (*curve_)[0];
    } else if (v >= static_cast<float>(curveSize) - 1) {
      channelData[i] = (*curve_)[curveSize - 1];
    } else {
      auto k = std::floor(v);
      auto f = v - k;
      auto kIndex = static_cast<size_t>(k);
      channelData[i] = (1 - f) * (*curve_)[kIndex] + f * (*curve_)[kIndex + 1];
    }
  }
}

void WaveShaper::processResampled(DSPAudioArray &channelData, int framesToProcess) {
  auto &outArray = (oversample_ == OverSampleType::OVERSAMPLE_4X) ? *tempBuffer4x_ : *tempBuffer2x_;
  const int outputFrames = upSampler_->process(channelData, framesToProcess, outArray);
  processNone(outArray, outputFrames);
  downSampler_->process(outArray, outputFrames, channelData);
}

} // namespace audioapi
