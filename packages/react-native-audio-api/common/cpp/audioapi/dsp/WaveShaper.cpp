#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/VectorMath.h>
#include <audioapi/dsp/WaveShaper.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

#include <algorithm>
#include <memory>
#include <string>

namespace audioapi {

WaveShaper::WaveShaper(const std::shared_ptr<AudioArray> &curve) : curve_(curve) {
  tempBuffer2x_ = std::make_shared<AudioArray>(RENDER_QUANTUM_SIZE * 2);
  tempBuffer2x_->zero();
  tempBuffer4x_ = std::make_shared<AudioArray>(RENDER_QUANTUM_SIZE * 4);
  tempBuffer4x_->zero();

  upSampler_ = std::make_unique<UpSampler>(RENDER_QUANTUM_SIZE, RENDER_QUANTUM_SIZE);
  downSampler_ = std::make_unique<DownSampler>(2 * RENDER_QUANTUM_SIZE, 2 * RENDER_QUANTUM_SIZE);
  upSampler2_ = std::make_unique<UpSampler>(2 * RENDER_QUANTUM_SIZE, RENDER_QUANTUM_SIZE);
  downSampler2_ = std::make_unique<DownSampler>(4 * RENDER_QUANTUM_SIZE, 2 * RENDER_QUANTUM_SIZE);
}

void WaveShaper::setCurve(const std::shared_ptr<AudioArray> &curve) {
  curve_ = curve;
}

void WaveShaper::setOversample(OverSampleType type) {
  oversample_ = type;

  if (upSampler_) {
    upSampler_->reset();
  }

  if (downSampler_) {
    downSampler_->reset();
  }

  if (upSampler2_) {
    upSampler2_->reset();
  }

  if (downSampler2_) {
    downSampler2_->reset();
  }
}

void WaveShaper::process(const std::shared_ptr<AudioArray> &channelData, int framesToProcess) {
  if (curve_ == nullptr) {
    return;
  }

  switch (oversample_) {
    case OverSampleType::OVERSAMPLE_2X:
      process2x(channelData, framesToProcess);
      break;
    case OverSampleType::OVERSAMPLE_4X:
      process4x(channelData, framesToProcess);
      break;
    case OverSampleType::OVERSAMPLE_NONE:
    default:
      processNone(channelData, framesToProcess);
      break;
  }
}

// based on https://webaudio.github.io/web-audio-api/#WaveShaperNode
void WaveShaper::processNone(const std::shared_ptr<AudioArray> &channelData, int framesToProcess) {
  auto curveArray = curve_->getData();
  auto curveSize = curve_->getSize();

  auto data = channelData->getData();

  for (int i = 0; i < framesToProcess; i++) {
    float v = (static_cast<float>(curveSize) - 1) * 0.5f * (data[i] + 1.0f);

    if (v <= 0) {
      data[i] = curveArray[0];
    } else if (v >= static_cast<float>(curveSize) - 1) {
      data[i] = curveArray[curveSize - 1];
    } else {
      auto k = std::floor(v);
      auto f = v - k;
      auto kIndex = static_cast<size_t>(k);
      data[i] = (1 - f) * curveArray[kIndex] + f * curveArray[kIndex + 1];
    }
  }
}

void WaveShaper::process2x(const std::shared_ptr<AudioArray> &channelData, int framesToProcess) {
  auto outputFrames = upSampler_->process(channelData, tempBuffer2x_, framesToProcess);
  processNone(tempBuffer2x_, outputFrames);
  downSampler_->process(tempBuffer2x_, channelData, outputFrames);
}

void WaveShaper::process4x(const std::shared_ptr<AudioArray> &channelData, int framesToProcess) {
  auto upSamplerOutputFrames = upSampler_->process(channelData, tempBuffer2x_, framesToProcess);
  auto upSampler2OutputFrames = upSampler2_->process(tempBuffer2x_, tempBuffer4x_, upSamplerOutputFrames);
  processNone(tempBuffer4x_, upSampler2OutputFrames);
  auto downSampler2OutputFrames = downSampler2_->process(tempBuffer4x_, tempBuffer2x_, upSampler2OutputFrames);
  downSampler_->process(tempBuffer2x_, channelData, downSampler2OutputFrames);
}

} // namespace audioapi
