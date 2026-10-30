#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>

#include <audioapi/utils/AudioArrayBuffer.hpp>

#include <cstddef>
#include <memory>

namespace audioapi {

AudioBufferHostObject::AudioBufferHostObject(const std::shared_ptr<AudioBuffer> &audioBuffer)
    : sharedBuffer_(std::make_shared<SharedAudioBuffer>(audioBuffer)) {
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

jsi::Object AudioBufferHostObject::createJsObject(
    jsi::Runtime &runtime,
    const std::shared_ptr<AudioBufferHostObject> &hostObject) {
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);
  object.setExternalMemoryPressure(runtime, hostObject->getSizeInBytes());
  return object;
}

jsi::Object AudioBufferHostObject::createJsObject(
    jsi::Runtime &runtime,
    const std::shared_ptr<AudioBuffer> &audioBuffer) {
  return createJsObject(runtime, std::make_shared<AudioBufferHostObject>(audioBuffer));
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, sampleRate) {
  return {sharedBuffer_->getSamples()->getSampleRate()};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, length) {
  return {static_cast<double>(sharedBuffer_->getSamples()->getSize())};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, duration) {
  return {sharedBuffer_->getSamples()->getDuration()};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, numberOfChannels) {
  return {static_cast<int>(sharedBuffer_->getSamples()->getNumberOfChannels())};
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, getChannelData) {
  auto audioArrayBuffer = sharedBuffer_->exposeChannel(static_cast<size_t>(args[0].getNumber()));
  auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);

  auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  return float32ArrayCtor.callAsConstructor(runtime, arrayBuffer).getObject(runtime);
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyFromChannel) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  sharedBuffer_->copyFromChannel(
      reinterpret_cast<float *>(arrayBuffer.data(runtime)),
      arrayBuffer.size(runtime) / sizeof(float),
      static_cast<size_t>(args[1].getNumber()),
      args[2].getNumber());

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyToChannel) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  sharedBuffer_->copyToChannel(
      reinterpret_cast<const float *>(arrayBuffer.data(runtime)),
      arrayBuffer.size(runtime) / sizeof(float),
      static_cast<size_t>(args[1].getNumber()),
      args[2].getNumber());

  return jsi::Value::undefined();
}

} // namespace audioapi
