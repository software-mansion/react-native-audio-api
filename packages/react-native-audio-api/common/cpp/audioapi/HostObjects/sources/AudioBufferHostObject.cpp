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
      immutableCopyCache_(std::move(other.immutableCopyCache_)),
      returnedChannelDataArrays_(std::move(other.returnedChannelDataArrays_)) {}

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
      audioBuffer_->detachSharedChannel(returned.channel);
      channelDetached[returned.channel] = true;
    }
  }

  returnedChannelDataArrays_.clear();
  // A view could have been written through right up until now, i.e. after the cached
  // copy was taken.
  immutableCopyCache_.invalidate();
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
  // The returned Float32Array is a live, JS-writable view straight into
  // audioBuffer_'s storage, so a copy cached before now can no longer be trusted.
  // Caching resumes once the view is neutralised by detachReturnedChannelData().
  immutableCopyCache_.invalidate();

  auto channel = static_cast<size_t>(args[0].getNumber());
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
  // Mutates audioBuffer_ in place, so any previously cached copy is now stale.
  immutableCopyCache_.invalidate();

  auto arrayBuffer =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto *source = reinterpret_cast<float *>(arrayBuffer.data(runtime));
  auto sourceLength = arrayBuffer.size(runtime) / sizeof(float);
  auto channelNumber = static_cast<int>(args[1].getNumber());
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
