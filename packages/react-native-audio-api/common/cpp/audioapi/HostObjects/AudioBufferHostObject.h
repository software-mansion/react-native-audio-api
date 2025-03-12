#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/jsi/AudioArrayBuffer.h>
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
    // this method could cause a crash if channelData is already deallocated,
    // but we handle deallocation internally so it should be safe
    auto channel = static_cast<int>(args[0].getNumber());
    auto channelData = reinterpret_cast<uint8_t *>(audioBuffer_->getChannelData(channel));
    auto length = static_cast<int>(audioBuffer_->getLength());
    auto size = static_cast<int>(length * sizeof(float));

    auto audioArrayBuffer = std::make_shared<AudioArrayBuffer>(channelData, size);
    auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);

    auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
    auto float32Array = float32ArrayCtor.callAsConstructor(runtime, arrayBuffer).getObject(runtime);

    return float32Array;
  }

  JSI_HOST_FUNCTION(copyFromChannel) {
    auto arrayBuffer = args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    auto destination = reinterpret_cast<float *>(arrayBuffer.data(runtime));
    auto length = static_cast<int>(arrayBuffer.size(runtime));
    auto channelNumber = static_cast<int>(args[1].getNumber());
    auto startInChannel = static_cast<size_t>(args[2].getNumber());

    audioBuffer_->copyFromChannel(
        destination, length, channelNumber, startInChannel);

    return jsi::Value::undefined();
  }

  JSI_HOST_FUNCTION(copyToChannel) {
    auto arrayBuffer = args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    auto source = reinterpret_cast<float *>(arrayBuffer.data(runtime));
    auto length = static_cast<int>(arrayBuffer.size(runtime));
    auto channelNumber = static_cast<int>(args[1].getNumber());
    auto startInChannel = static_cast<size_t>(args[2].getNumber());

    audioBuffer_->copyToChannel(
        source, length, channelNumber, startInChannel);

    return jsi::Value::undefined();
  }
};
} // namespace audioapi
