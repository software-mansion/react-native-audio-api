#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>

namespace audioapi {

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
  auto channelData =
      reinterpret_cast<uint8_t *>(audioBuffer_->getChannelData(channel));
  auto length = static_cast<int>(audioBuffer_->getLength());
  auto size = static_cast<int>(length * sizeof(float));

  // reading or writing from this ArrayBuffer could cause a crash
  // if underlying channelData is deallocated
  auto audioArrayBuffer = std::make_shared<AudioArrayBuffer>(channelData, size);
  auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);

  auto float32ArrayCtor =
      runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  auto float32Array = float32ArrayCtor.callAsConstructor(runtime, arrayBuffer)
                          .getObject(runtime);

  return float32Array;
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyFromChannel) {
  auto arrayBuffer = args[0]
                         .getObject(runtime)
                         .getPropertyAsObject(runtime, "buffer")
                         .getArrayBuffer(runtime);
  auto destination = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));
  auto channelNumber = static_cast<int>(args[1].getNumber());
  auto startInChannel = static_cast<size_t>(args[2].getNumber());

  audioBuffer_->copyFromChannel(
      destination, length, channelNumber, startInChannel);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyToChannel) {
  auto arrayBuffer = args[0]
                         .getObject(runtime)
                         .getPropertyAsObject(runtime, "buffer")
                         .getArrayBuffer(runtime);
  auto source = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto length = static_cast<int>(arrayBuffer.size(runtime));
  auto channelNumber = static_cast<int>(args[1].getNumber());
  auto startInChannel = static_cast<size_t>(args[2].getNumber());

  audioBuffer_->copyToChannel(source, length, channelNumber, startInChannel);

  return jsi::Value::undefined();
}

} // namespace audioapi
