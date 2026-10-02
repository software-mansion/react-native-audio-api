#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>

#include <audioapi/utils/AudioArrayBuffer.hpp>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

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
    : HostObject(std::move(other)),
      audioBuffer_(std::move(other.audioBuffer_)),
      channelSharedWithNode_(std::move(other.channelSharedWithNode_)),
      contentVersion_(other.contentVersion_),
      returnedChannelDataArrays_(std::move(other.returnedChannelDataArrays_)) {}

std::shared_ptr<AudioBuffer> AudioBufferHostObject::shareForPlayback() {
  channelSharedWithNode_.assign(audioBuffer_->getNumberOfChannels(), true);
  return audioBuffer_->shareChannels();
}

void AudioBufferHostObject::makeChannelWritable(size_t channel) {
  if (channel < channelSharedWithNode_.size() && channelSharedWithNode_[channel]) {
    replaceChannelStorage(channel);
  }
}

void AudioBufferHostObject::replaceChannelStorage(size_t channel) {
  // mark the channel as no longer shared with a node, so that future writes to it don't trigger another copy-on-write
  if (channel < channelSharedWithNode_.size()) {
    audioBuffer_->detachSharedChannel(channel);
    channelSharedWithNode_[channel] = false;
    ++contentVersion_;
  }
}

void AudioBufferHostObject::detachReturnedChannelData(jsi::Runtime &runtime) {
  if (returnedChannelDataArrays_.empty()) {
    return;
  }

  auto defineProperty = runtime.global()
                            .getPropertyAsObject(runtime, "Object")
                            .getPropertyAsFunction(runtime, "defineProperty");
  auto zeroDescriptor = jsi::Object(runtime);
  zeroDescriptor.setProperty(runtime, "value", 0);

  std::vector<bool> channelDetached(audioBuffer_->getNumberOfChannels(), false);

  for (const auto &returned : returnedChannelDataArrays_) {
    auto array = returned.array.lock(runtime);
    if (array.isObject()) {
      for (const auto *sizeProperty : {"length", "byteLength", "byteOffset"}) {
        defineProperty.call(runtime, array, sizeProperty, zeroDescriptor);
      }
    }

    if (!channelDetached[returned.channel]) {
      replaceChannelStorage(returned.channel);
      channelDetached[returned.channel] = true;
    }
  }

  returnedChannelDataArrays_.clear();
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, sampleRate) {
  return {audioBuffer_->getSampleRate()};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, length) {
  return {static_cast<double>(audioBuffer_->getSize())};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, duration) {
  return {audioBuffer_->getDuration()};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferHostObject, numberOfChannels) {
  return {static_cast<int>(audioBuffer_->getNumberOfChannels())};
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, getChannelData) {
  auto channel = static_cast<size_t>(args[0].getNumber());
  // The returned Float32Array is a live, JS-writable view straight into audioBuffer_'s
  // storage, so that storage must not be one a node is playing.
  makeChannelWritable(channel);

  auto audioArrayBuffer = audioBuffer_->getSharedChannel(channel);
  auto arrayBuffer = jsi::ArrayBuffer(runtime, audioArrayBuffer);

  auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  auto float32Array = float32ArrayCtor.callAsConstructor(runtime, arrayBuffer).getObject(runtime);

  float32Array.setExternalMemoryPressure(runtime, audioArrayBuffer->size());
  returnedChannelDataArrays_.push_back(
      {.channel = channel, .array = jsi::WeakObject(runtime, float32Array)});

  return float32Array;
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyFromChannel) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto *destination = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto destinationLength = arrayBuffer.size(runtime) / sizeof(float);
  auto channelNumber = static_cast<int>(args[1].getNumber());
  auto rawStart = args[2].getNumber();
  auto channelSize = audioBuffer_->getSize();

  // Per spec, an out-of-range (or negative) startInChannel copies nothing, and a
  // copy that would run past the end of the channel is truncated rather than throwing.
  if (rawStart >= 0 && static_cast<size_t>(rawStart) < channelSize) {
    auto startInChannel = static_cast<size_t>(rawStart);
    auto framesToCopy = std::min(destinationLength, channelSize - startInChannel);
    audioBuffer_->getChannel(channelNumber)->copyTo(destination, startInChannel, 0, framesToCopy);
  }

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferHostObject, copyToChannel) {
  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto *source = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto sourceLength = arrayBuffer.size(runtime) / sizeof(float);
  auto channelNumber = static_cast<int>(args[1].getNumber());
  // Mutates audioBuffer_ in place, so that channel must not be one a node is playing.
  makeChannelWritable(static_cast<size_t>(channelNumber));
  auto rawStart = args[2].getNumber();
  auto channelSize = audioBuffer_->getSize();

  // Per spec, an out-of-range (or negative) startInChannel copies nothing, and a
  // copy that would run past the end of the channel is truncated rather than throwing.
  if (rawStart >= 0 && static_cast<size_t>(rawStart) < channelSize) {
    auto startInChannel = static_cast<size_t>(rawStart);
    auto framesToCopy = std::min(sourceLength, channelSize - startInChannel);
    audioBuffer_->getChannel(channelNumber)->copy(source, 0, startInChannel, framesToCopy);
  }

  return jsi::Value::undefined();
}

} // namespace audioapi
