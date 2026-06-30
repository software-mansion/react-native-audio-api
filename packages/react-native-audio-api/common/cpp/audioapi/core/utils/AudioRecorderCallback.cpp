
#include <audioapi/core/utils/AudioRecorderCallback.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/events/AudioEventHandlerRegistry.h>
#include <audioapi/utils/CircularArray.hpp>

#include <audioapi/events/AudioEventPayload.h>
#include <algorithm>
#include <memory>
#include <string>

namespace audioapi {

/// @brief Constructor
/// Allocates circular buffer (as every property to do so is already known at this point).
/// @param audioEventHandlerRegistry The audio event handler registry
/// @param sampleRate The user desired sample rate
/// @param bufferLength The user desired buffer length
/// @param channelCount The user desired channel count
/// @param callbackId The callback identifier
AudioRecorderCallback::AudioRecorderCallback(
    const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId)
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      channelCount_(channelCount),
      audioReadyEvent_(audioEventHandlerRegistry),
      errorEvent_(audioEventHandlerRegistry) {
  audioReadyEvent_.assignCallbackId(callbackId);
  ringBufferSize_ = std::max(bufferLength * 2, static_cast<size_t>(DEFAULT_RING_BUFFER_SIZE));
  circularBuffer_.resize(channelCount_);

  for (size_t i = 0; i < channelCount_; ++i) {
    circularBuffer_[i] = std::make_shared<CircularAudioArray>(ringBufferSize_);
  }

  isInitialized_.store(true, std::memory_order_release);
}

AudioRecorderCallback::~AudioRecorderCallback() {
  isInitialized_.store(false, std::memory_order_release);
}

/// @brief Emits audio data from the circular buffer when enough frames are available.
/// @param flush If true, emits all available data regardless of buffer length.
void AudioRecorderCallback::emitAudioData(bool flush) {
  size_t sizeLimit = flush ? circularBuffer_[0]->getNumberOfAvailableFrames() : bufferLength_;

  if (sizeLimit == 0) {
    return;
  }

  while (circularBuffer_[0]->getNumberOfAvailableFrames() >= sizeLimit) {
    auto buffer = std::make_shared<AudioBuffer>(sizeLimit, channelCount_, sampleRate_);

    for (int i = 0; i < channelCount_; ++i) {
      circularBuffer_[i]->pop_front(*buffer->getChannel(i), sizeLimit);
    }

    invokeCallback(buffer, static_cast<int>(sizeLimit));
  }
}

void AudioRecorderCallback::invokeCallback(
    const std::shared_ptr<AudioBuffer> &buffer,
    int numFrames) {
  audioReadyEvent_.dispatch(
      AudioReadyPayload{
          .buffer = std::make_shared<AudioBufferHostObject>(buffer),
          .numFrames = numFrames,
          .when = static_cast<double>(framesEmitted_) / sampleRate_});
  framesEmitted_ += numFrames;
}

void AudioRecorderCallback::assignOnErrorCallbackId(uint64_t callbackId) {
  errorEvent_.assignCallbackId(callbackId);
}

/// @brief Invokes the error callback with the provided message.
/// @param message The error message to be sent to the callback.
void AudioRecorderCallback::invokeOnErrorCallback(const std::string &message) {
  errorEvent_.dispatch(StringPayload{.name = "message", .reason = message});
}

} // namespace audioapi
