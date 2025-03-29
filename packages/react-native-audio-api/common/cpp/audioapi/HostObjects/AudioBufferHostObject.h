#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/core/sources/AudioBuffer.h>

#include <jsi/jsi.h>
#include <memory>
#include <vector>
#include <cstddef>

namespace audioapi {
using namespace facebook;

class AudioBufferHostObject : public JsiHostObject {
 public:
  std::shared_ptr<AudioBuffer> audioBuffer_;

  explicit AudioBufferHostObject(
      const std::shared_ptr<AudioBuffer> &audioBuffer)
      : audioBuffer_(audioBuffer) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, sampleRate),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, length),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, duration),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, numberOfChannels));

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioBufferHostObject, getChannelData),
        JSI_EXPORT_FUNCTION(AudioBufferHostObject, copyFromChannel),
        JSI_EXPORT_FUNCTION(AudioBufferHostObject, copyToChannel));
  }

  JSI_PROPERTY_GETTER(sampleRate) {
    return {audioBuffer_->getSampleRate()};
  }

  JSI_PROPERTY_GETTER(length) {
    return {static_cast<double>(audioBuffer_->getLength())};
  }

  JSI_PROPERTY_GETTER(duration) {
    return {audioBuffer_->getDuration()};
  }

  JSI_PROPERTY_GETTER(numberOfChannels) {
    return {audioBuffer_->getNumberOfChannels()};
  }

  JSI_HOST_FUNCTION(getChannelData) {
    int channel = static_cast<int>(args[0].getNumber());
    float *channelData = audioBuffer_->getChannelData(channel);

    auto array = jsi::Array(runtime, audioBuffer_->getLength());
    for (int i = 0; i < audioBuffer_->getLength(); i++) {
      array.setValueAtIndex(runtime, i, jsi::Value(channelData[i]));
    }

    return array;
  }

  JSI_HOST_FUNCTION(copyFromChannel) {
    auto destination = args[0].getObject(runtime).asArray(runtime);
    auto destinationLength =
        static_cast<size_t>(destination.getProperty(runtime, "length").asNumber());
    auto channelNumber = static_cast<int>(args[1].getNumber());
    auto startInChannel = static_cast<size_t>(args[2].getNumber());

    auto *destinationData = new float[destinationLength];

    audioBuffer_->copyFromChannel(
        destinationData, destinationLength, channelNumber, startInChannel);

    for (int i = 0; i < destinationLength; i++) {
      destination.setValueAtIndex(runtime, i, jsi::Value(destinationData[i]));
    }

    delete[] destinationData;
    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(copyToChannel) {
    auto source = args[0].getObject(runtime).asArray(runtime);
    auto sourceLength =
        static_cast<size_t>(source.getProperty(runtime, "length").asNumber());
    auto channelNumber = static_cast<int>(args[1].getNumber());
    auto startInChannel = static_cast<size_t>(args[2].getNumber());

    auto *sourceData = new float[sourceLength];

    for (int i = 0; i < sourceLength; i++) {
      sourceData[i] =
          static_cast<float>(source.getValueAtIndex(runtime, i).getNumber());
    }

    audioBuffer_->copyToChannel(
        sourceData, sourceLength, channelNumber, startInChannel);

    delete[] sourceData;
    return jsi::Value::undefined();
  }
};
} // namespace audioapi
