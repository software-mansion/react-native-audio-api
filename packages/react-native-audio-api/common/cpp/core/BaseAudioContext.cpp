#ifdef ANDROID
#include "AudioPlayer.h"
#else
#include "IOSAudioDecoder.h"
#include "IOSAudioPlayer.h"
#endif

#include "BaseAudioContext.h"

#include "AudioArray.h"
#include "AudioBuffer.h"
#include "AudioBufferSourceNode.h"
#include "AudioBus.h"
#include "AudioDestinationNode.h"
#include "AudioNodeManager.h"
#include "BiquadFilterNode.h"
#include "ContextState.h"
#include "GainNode.h"
#include "OscillatorNode.h"
#include "StereoPannerNode.h"

namespace audioapi {

BaseAudioContext::BaseAudioContext() {
#ifdef ANDROID
  audioPlayer_ = std::make_shared<AudioPlayer>(this->renderAudio());
#else
  audioPlayer_ = std::make_shared<IOSAudioPlayer>(this->renderAudio());
  audioDecoder_ =
      std::make_shared<IOSAudioDecoder>(audioPlayer_->getSampleRate());
#endif

  sampleRate_ = audioPlayer_->getSampleRate();
  bufferSizeInFrames_ = audioPlayer_->getBufferSizeInFrames();

  audioPlayer_->start();
  nodeManager_ = std::make_shared<AudioNodeManager>();
  destination_ = std::make_shared<AudioDestinationNode>(this);
}

std::string BaseAudioContext::getState() {
  return BaseAudioContext::toString(state_);
}

int BaseAudioContext::getSampleRate() const {
  return sampleRate_;
}

int BaseAudioContext::getBufferSizeInFrames() const {
  return bufferSizeInFrames_;
}

std::size_t BaseAudioContext::getCurrentSampleFrame() const {
  return destination_->getCurrentSampleFrame();
}

double BaseAudioContext::getCurrentTime() const {
  return destination_->getCurrentTime();
}

std::shared_ptr<AudioDestinationNode> BaseAudioContext::getDestination() {
  return destination_;
}

std::shared_ptr<OscillatorNode> BaseAudioContext::createOscillator() {
  return std::make_shared<OscillatorNode>(this);
}

std::shared_ptr<GainNode> BaseAudioContext::createGain() {
  return std::make_shared<GainNode>(this);
}

std::shared_ptr<StereoPannerNode> BaseAudioContext::createStereoPanner() {
  return std::make_shared<StereoPannerNode>(this);
}

std::shared_ptr<BiquadFilterNode> BaseAudioContext::createBiquadFilter() {
  return std::make_shared<BiquadFilterNode>(this);
}

std::shared_ptr<AudioBufferSourceNode> BaseAudioContext::createBufferSource() {
  return std::make_shared<AudioBufferSourceNode>(this);
}

std::shared_ptr<AudioBuffer> BaseAudioContext::createBuffer(
    int numberOfChannels,
    int length,
    int sampleRate) {
  return std::make_shared<AudioBuffer>(numberOfChannels, length, sampleRate);
}

std::shared_ptr<PeriodicWave> BaseAudioContext::createPeriodicWave(
    float *real,
    float *imag,
    bool disableNormalization,
    int length) {
  return std::make_shared<PeriodicWave>(
      sampleRate_, real, imag, length, disableNormalization);
}

#ifdef ANDROID
std::shared_ptr<AudioBuffer> BaseAudioContext::decodeAudioDataSource(
    std::string source) {
  return {nullptr};
}
#else
std::shared_ptr<AudioBuffer> BaseAudioContext::decodeAudioDataSource(
    std::string source) {
  auto audioBus = audioDecoder_->decodeWithFilePath(source);
  return std::make_shared<AudioBuffer>(audioBus);
}
#endif

std::function<void(AudioBus *, int)> BaseAudioContext::renderAudio() {
  if (state_ == ContextState::CLOSED) {
    return [](AudioBus *, int) {};
  }

  return [this](AudioBus *data, int frames) {
    destination_->renderAudio(data, frames);
  };
}

AudioNodeManager *BaseAudioContext::getNodeManager() {
  return nodeManager_.get();
}

std::string BaseAudioContext::toString(ContextState state) {
  switch (state) {
    case ContextState::SUSPENDED:
      return "suspended";
    case ContextState::RUNNING:
      return "running";
    case ContextState::CLOSED:
      return "closed";
    default:
      throw std::invalid_argument("Unknown context state");
  }
}

std::shared_ptr<PeriodicWave> BaseAudioContext::getBasicWaveForm(
    OscillatorType type) {
  switch (type) {
    case OscillatorType::SINE:
      if (cachedSineWave_ == nullptr) {
        cachedSineWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedSineWave_;
    case OscillatorType::SQUARE:
      if (cachedSquareWave_ == nullptr) {
        cachedSquareWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedSquareWave_;
    case OscillatorType::SAWTOOTH:
      if (cachedSawtoothWave_ == nullptr) {
        cachedSawtoothWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedSawtoothWave_;
    case OscillatorType::TRIANGLE:
      if (cachedTriangleWave_ == nullptr) {
        cachedTriangleWave_ =
            std::make_shared<PeriodicWave>(sampleRate_, type, false);
      }
      return cachedTriangleWave_;
    case OscillatorType::CUSTOM:
      throw std::invalid_argument(
          "You can't get a custom wave form. You need to create it.");
      break;
  }
}

} // namespace audioapi
