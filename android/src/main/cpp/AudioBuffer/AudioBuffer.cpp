#include "AudioBuffer.h"

namespace audiocontext {

AudioBuffer::AudioBuffer(jni::alias_ref<AudioBuffer::jhybridobject> &jThis)
    : javaPart_(make_global(jThis)) {}

int AudioBuffer::getSampleRate() {
  static const auto method =
      javaClassStatic()->getMethod<jint()>("getSampleRate");
  return method(javaPart_);
}

int AudioBuffer::getLength() {
  static const auto method = javaClassStatic()->getMethod<jint()>("getLength");
  return method(javaPart_);
}

double AudioBuffer::getDuration() {
  static const auto method =
      javaClassStatic()->getMethod<jdouble()>("getDuration");
  return method(javaPart_);
}

int AudioBuffer::getNumberOfChannels() {
  static const auto method =
      javaClassStatic()->getMethod<jint()>("getNumberOfChannels");
  return method(javaPart_);
}

short **AudioBuffer::getChannelData(int channel) {
  static const auto method =
      javaClassStatic()->getMethod<JArrayShort(jint)>("getChannelData");
  auto jArray = method(javaPart_, channel);
  auto length = jArray->size();

  auto channelData = new short *[length];
  auto pin = jArray->pin();
  for (int i = 0; i < length; i++) {
    channelData[i] = &pin[i];
  }

  return channelData;
}

void AudioBuffer::setChannelData(int channel, short **data) {
  static const auto method =
      javaClassStatic()->getMethod<void(jint, jshortArray)>("setChannelData");
  std::vector<jshort> buffer(getLength());
  for (int i = 0; i < getLength(); i++) {
    buffer[i] = *data[i];
  }
  auto jArray = JArrayShort::newArray(getLength());
  jArray->setRegion(0, getLength(), buffer.data());

  method(javaPart_, channel, jArray.get());
}

void AudioBuffer::prepareForDeconstruction() {
  javaPart_.reset();
}

} // namespace audiocontext
