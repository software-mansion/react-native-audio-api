#include "AudioBuffer.h"

namespace audioapi {

AudioBuffer::AudioBuffer(jni::alias_ref<AudioBuffer::jhybridobject> &jThis)
    : javaPart_(make_global(jThis)) {}

int AudioBuffer::getSampleRate() const {
  static const auto method =
      javaClassStatic()->getMethod<jint()>("getSampleRate");
  return method(javaPart_);
}

int AudioBuffer::getLength() const {
  static const auto method = javaClassStatic()->getMethod<jint()>("getLength");
  return method(javaPart_);
}

double AudioBuffer::getDuration() const {
  static const auto method =
      javaClassStatic()->getMethod<jdouble()>("getDuration");
  return method(javaPart_);
}

int AudioBuffer::getNumberOfChannels() const {
  static const auto method =
      javaClassStatic()->getMethod<jint()>("getNumberOfChannels");
  return method(javaPart_);
}

float *AudioBuffer::getChannelData(int channel) const {
  static const auto method =
      javaClassStatic()->getMethod<JArrayFloat(jint)>("getChannelData");
  auto jArray = method(javaPart_, channel);
  auto length = jArray->size();

  auto channelData = new float[length];
  auto pin = jArray->pin();
  for (int i = 0; i < length; i++) {
    channelData[i] = pin[i];
  }

  return channelData;
}

void AudioBuffer::setChannelData(int channel, const float *data) {
  static const auto method =
      javaClassStatic()->getMethod<void(jint, jfloatArray)>("setChannelData");
  std::vector<jfloat> buffer(getLength());
  for (int i = 0; i < getLength(); i++) {
    buffer[i] = data[i];
  }
  auto jArray = JArrayFloat::newArray(getLength());
  jArray->setRegion(0, getLength(), buffer.data());

  method(javaPart_, channel, jArray.get());
}

void AudioBuffer::prepareForDeconstruction() {
  javaPart_.reset();
}

} // namespace audioapi
