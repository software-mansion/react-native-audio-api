#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>

#include <audioapi/jsi/AudioArrayBuffer.h>
#include <audioapi/utils/AudioBus.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioBufferHostObject::AudioBufferHostObject(const std::shared_ptr<AudioBuffer> &audioBuffer)
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

AudioBufferHostObject::AudioBufferHostObject(AudioBufferHostObject &&other) noexcept
    : JsiHostObject(std::move(other)), audioBuffer_(std::move(other.audioBuffer_)) {}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, sampleRate) {
  return {audioBuffer_->getSampleRate()};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, length) {
  return {static_cast<double>(audioBuffer_->getLength())};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, duration) {
  return {audioBuffer_->getDuration()};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, numberOfChannels) {
  return {audioBuffer_->getNumberOfChannels()};
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, getChannelData) {
  auto channel = static_cast<int>(args[0].getNumber());
  auto audioArrayBuffer =
      std::make_shared<AudioArrayBuffer>(audioBuffer_->bus_->getSharedChannel(channel));
  auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);

  auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  auto float32Array = float32ArrayCtor.callAsConstructor(runtime, arrayBuffer).getObject(runtime);

  float32Array.setExternalMemoryPressure(runtime, audioArrayBuffer->size());

  return float32Array;
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyFromChannel) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto destination = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));
  auto channelNumber = static_cast<int>(args[1].getNumber());
  auto startInChannel = static_cast<size_t>(args[2].getNumber());

  audioBuffer_->copyFromChannel(destination, length, channelNumber, startInChannel);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyToChannel) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto source = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));
  auto channelNumber = static_cast<int>(args[1].getNumber());
  auto startInChannel = static_cast<size_t>(args[2].getNumber());

  audioBuffer_->copyToChannel(source, length, channelNumber, startInChannel);

  return jsi::Value::undefined();
}

} // namespace audioapi
