#include "AnalyserNode.h"
#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"

namespace audioapi {
AnalyserNode::AnalyserNode(audioapi::BaseAudioContext *context)
    : AudioNode(context) {
  channelCount_ = 1;
  fftSize_ = 2048;
  minDecibels_ = -100;
  maxDecibels_ = -30;
  smoothingTimeConstant_ = 0.8;
  inputBus_ =
      std::make_unique<AudioBus>(context->getSampleRate(), MAX_FFT_SIZE * 2, 1);
}

int AnalyserNode::getFftSize() const {
  return fftSize_;
}

int AnalyserNode::getFrequencyBinCount() const {
  return fftSize_ / 2;
}

double AnalyserNode::getMinDecibels() const {
  return minDecibels_;
}

double AnalyserNode::getMaxDecibels() const {
  return maxDecibels_;
}

double AnalyserNode::getSmoothingTimeConstant() const {
  return smoothingTimeConstant_;
}

void AnalyserNode::setFftSize(int fftSize) {
  fftSize_ = fftSize;
}

void AnalyserNode::setMinDecibels(double minDecibels) {
  minDecibels_ = minDecibels;
}

void AnalyserNode::setMaxDecibels(double maxDecibels) {
  maxDecibels_ = maxDecibels;
}

void AnalyserNode::setSmoothingTimeConstant(double smoothingTimeConstant) {
  smoothingTimeConstant_ = smoothingTimeConstant;
}

float *AnalyserNode::getFloatFrequencyData() {
  return nullptr;
}

uint8_t *AnalyserNode::getByteFrequencyData() {
  return nullptr;
}

float *AnalyserNode::getFloatTimeDomainData() {
  return nullptr;
}

uint8_t *AnalyserNode::getByteTimeDomainData() {
  return nullptr;
}

void AnalyserNode::processNode(
    audioapi::AudioBus *processingBus,
    int framesToProcess) {
  // m_analyser.writeInput(inputBus, framesToProcess);
}
} // namespace audioapi
