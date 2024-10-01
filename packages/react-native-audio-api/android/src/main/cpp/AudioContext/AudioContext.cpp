#include "AudioContext.h"

namespace audioapi {

using namespace facebook::jni;

AudioContext::AudioContext(jni::alias_ref<AudioContext::jhybridobject> &jThis)
    : javaPart_(make_global(jThis)) {}

std::shared_ptr<AudioDestinationNode> AudioContext::getDestination() {
  return std::make_shared<AudioDestinationNode>();
}

std::shared_ptr<OscillatorNode> AudioContext::createOscillator() {
  return std::make_shared<OscillatorNode>();
}

std::shared_ptr<GainNode> AudioContext::createGain() {
  return std::make_shared<GainNode>();
}

std::shared_ptr<StereoPannerNode> AudioContext::createStereoPanner() {
  return std::make_shared<StereoPannerNode>();
}

std::shared_ptr<BiquadFilterNode> AudioContext::createBiquadFilter() {
  return std::make_shared<BiquadFilterNode>();
}

std::shared_ptr<AudioBufferSourceNode> AudioContext::createBufferSource() {
  return std::make_shared<AudioBufferSourceNode>();
}

AudioBuffer *
AudioContext::createBuffer(int numberOfChannels, int length, int sampleRate) {
  static const auto method =
      javaClassLocal()->getMethod<AudioBuffer(int, int, int)>("createBuffer");
  auto buffer = method(javaPart_.get(), numberOfChannels, length, sampleRate);

  return buffer->cthis();
}

std::string AudioContext::getState() {
  static const auto method = javaClassLocal()->getMethod<JString()>("getState");
  return method(javaPart_.get())->toStdString();
}

int AudioContext::getSampleRate() {
  static const auto method =
      javaClassLocal()->getMethod<jint()>("getSampleRate");
  return method(javaPart_.get());
}

double AudioContext::getCurrentTime() {
  static const auto method =
      javaClassLocal()->getMethod<jdouble()>("getCurrentTime");
  return method(javaPart_.get());
}

void AudioContext::close() {
  static const auto method = javaClassLocal()->getMethod<void()>("close");
  method(javaPart_.get());
  javaPart_.reset();
}
} // namespace audioapi
