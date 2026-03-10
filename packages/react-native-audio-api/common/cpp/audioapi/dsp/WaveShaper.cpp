#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/dsp/WaveShaper.h>
#include <audioapi/dsp/r8brain/Resampler.h>

#include <cstring>
#include <memory>

namespace audioapi {

WaveShaper::WaveShaper(const std::shared_ptr<AudioArray> &curve, float sampleRate)
    : curve_(curve), sampleRate_(sampleRate) {
  tempBuffer2x_ = std::make_shared<AudioArray>(RENDER_QUANTUM_SIZE * 2);
  tempBuffer2x_->zero();
  tempBuffer4x_ = std::make_shared<AudioArray>(RENDER_QUANTUM_SIZE * 4);
  tempBuffer4x_->zero();

  createResamplers(OverSampleType::OVERSAMPLE_NONE);
}

void WaveShaper::createResamplers(OverSampleType type) {
  if (type == OverSampleType::OVERSAMPLE_2X) {
    upSampler_ = std::make_unique<r8b::SingleChannelResampler>(
        sampleRate_, sampleRate_ * 2, RENDER_QUANTUM_SIZE);
    downSampler_ = std::make_unique<r8b::SingleChannelResampler>(
        sampleRate_ * 2, sampleRate_, RENDER_QUANTUM_SIZE * 2);
  } else if (type == OverSampleType::OVERSAMPLE_4X) {
    upSampler_ = std::make_unique<r8b::SingleChannelResampler>(
        sampleRate_, sampleRate_ * 4, RENDER_QUANTUM_SIZE * 2);
    downSampler_ = std::make_unique<r8b::SingleChannelResampler>(
        sampleRate_ * 4, sampleRate_, RENDER_QUANTUM_SIZE * 4);
  }
}

void WaveShaper::setCurve(const std::shared_ptr<AudioArray> &curve) {
  curve_ = curve;
}

void WaveShaper::setOversample(OverSampleType type) {
  oversample_ = type;
  createResamplers(type);
}

void WaveShaper::process(AudioArray &channelData, int framesToProcess) {
  if (curve_ == nullptr) {
    return;
  }

  switch (oversample_) {
    case OverSampleType::OVERSAMPLE_2X:
      processResampled(channelData, framesToProcess);
      break;
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
void WaveShaper::processNone(AudioArray &channelData, int framesToProcess) {
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

void WaveShaper::processResampled(AudioArray &channelData, int framesToProcess) {
  AudioArray &outArray =
      (oversample_ == OverSampleType::OVERSAMPLE_4X) ? *tempBuffer4x_ : *tempBuffer2x_;
  const int outputFrames = upSampler_->process(channelData, framesToProcess, outArray);
  processNone(outArray, outputFrames);
  downSampler_->process(outArray, outputFrames, channelData);
}

} // namespace audioapi
