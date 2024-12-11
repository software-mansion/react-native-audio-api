#include "AudioBufferHostObject.h"

namespace audioapi {
using namespace facebook;

AudioBufferHostObject::AudioBufferHostObject(
    const std::shared_ptr<AudioBuffer> &audioBuffer)
    : audioBuffer_(audioBuffer) {}

std::vector<jsi::PropNameID> AudioBufferHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "buffer"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "sampleRate"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "length"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "duration"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "numberOfChannels"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "getChannelData"));
  propertyNames.push_back(
      jsi::PropNameID::forAscii(runtime, "copyFromChannel"));
  propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "copyToChannel"));
  return propertyNames;
}

jsi::Value AudioBufferHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "sampleRate") {
    return {audioBuffer_->getSampleRate()};
  }

  if (propName == "length") {
    return {audioBuffer_->getLength()};
  }

  if (propName == "duration") {
    return {audioBuffer_->getDuration()};
  }

  if (propName == "numberOfChannels") {
    return {audioBuffer_->getNumberOfChannels()};
  }

  if (propName == "getChannelData") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        1,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          int channel = static_cast<int>(args[0].getNumber());
          float *channelData = audioBuffer_->getChannelData(channel);

          auto array = jsi::Array(rt, audioBuffer_->getLength());
          for (int i = 0; i < audioBuffer_->getLength(); i++) {
            array.setValueAtIndex(rt, i, jsi::Value(channelData[i]));
          }

          return array;
        });
  }

  if (propName == "copyFromChannel") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        3,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto destination = args[0].getObject(rt).asArray(rt);
          auto destinationLength = static_cast<int>(
              destination.getProperty(rt, "length").asNumber());
          auto channelNumber = static_cast<int>(args[1].getNumber());
          auto startInChannel = static_cast<int>(args[2].getNumber());

          auto *destinationData = new float[destinationLength];

          audioBuffer_->copyFromChannel(
              destinationData,
              destinationLength,
              channelNumber,
              startInChannel);

          for (int i = 0; i < destinationLength; i++) {
            destination.setValueAtIndex(rt, i, jsi::Value(destinationData[i]));
          }

          return jsi::Value::undefined();
        });
  }

  if (propName == "copyToChannel") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        3,
        [this](
            jsi::Runtime &rt,
            const jsi::Value &thisVal,
            const jsi::Value *args,
            size_t count) -> jsi::Value {
          auto source = args[0].getObject(rt).asArray(rt);
          auto sourceLength =
              static_cast<int>(source.getProperty(rt, "length").asNumber());
          auto channelNumber = static_cast<int>(args[1].getNumber());
          auto startInChannel = static_cast<int>(args[2].getNumber());

          auto *sourceData = new float[sourceLength];

          for (int i = 0; i < sourceLength; i++) {
            sourceData[i] =
                static_cast<float>(source.getValueAtIndex(rt, i).getNumber());
          }

          audioBuffer_->copyToChannel(
              sourceData, sourceLength, channelNumber, startInChannel);

          return jsi::Value::undefined();
        });
  }

  // `decodeAudioData` is a method that returns a promise to
  // AudioBufferHostObject It seems that async/await checks for the presence of
  // `then` method on the object
  if (propName == "then") {
    return jsi::Value::undefined();
  }

  throw std::runtime_error("Not yet implemented!");
}

void AudioBufferHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  throw std::runtime_error("Not yet implemented!");
}

} // namespace audioapi
