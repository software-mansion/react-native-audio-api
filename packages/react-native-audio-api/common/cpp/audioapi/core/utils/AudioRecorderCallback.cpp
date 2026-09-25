#include <audioapi/core/utils/AudioRecorderCallback.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/CircularArray.hpp>

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace audioapi {

/// @brief Constructor
/// Allocates circular buffer (as every property to do so is already known at this point).
/// @param audioEventHandlerRegistry The audio event handler registry
/// @param sampleRate The user desired sample rate
/// @param bufferLength The user desired buffer length
/// @param channelCount The user desired channel count
/// @param callbackId The callback identifier
AudioRecorderCallback::AudioRecorderCallback(
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
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

  for (size_t i = 0; i < circularBuffer_.size(); ++i) {
    circularBuffer_[i] = std::make_shared<CircularAudioArray>(ringBufferSize_);
  }

  isInitialized_.store(true, std::memory_order_release);
}

AudioRecorderCallback::~AudioRecorderCallback() {
  isInitialized_.store(false, std::memory_order_release);
  cleanup();
}

/// @brief Prepares the callback by initializing the data converter and allocating buffers.
/// @param streamSampleRate The sample rate of the incoming audio stream.
/// @param streamChannelCount The channel count of the incoming audio stream.
/// @param maxInputBufferLength The maximum buffer length of the incoming audio stream.
Result<NoneType, std::string> AudioRecorderCallback::prepare(
    float streamSampleRate,
    int streamChannelCount,
    size_t maxInputBufferLength) {
  streamSampleRate_ = streamSampleRate;
  streamChannelCount_ = streamChannelCount;
  maxInputBufferLength_ = maxInputBufferLength;

  if (streamSampleRate_ <= 0 || streamChannelCount_ <= 0 || maxInputBufferLength_ == 0) {
    return Result<NoneType, std::string>::Err("Invalid stream sample rate or channel count");
  }

  if (sampleRate_ <= 0 || channelCount_ <= 0) {
    return Result<NoneType, std::string>::Err("Invalid callback sample rate or channel count");
  }
  if (streamChannelCount_ > MAX_CHANNEL_COUNT || channelCount_ > MAX_CHANNEL_COUNT) {
    return Result<NoneType, std::string>::Err("Channel count exceeds MAX_CHANNEL_COUNT");
  }

  const bool needsRemix = streamChannelCount_ != channelCount_;
  const bool needsResampling = streamSampleRate_ != sampleRate_;

  if (needsRemix || needsResampling) {
    const auto chunkFrames = static_cast<size_t>(RESAMPLER_MAX_INPUT_FRAMES);

    if (needsRemix) {
      remixedChunk_ = std::make_unique<AudioBuffer>(chunkFrames, channelCount_, streamSampleRate_);
    }

    if (needsResampling) {
      resampler_ = std::make_unique<r8b::MultiChannelResampler>(
          streamSampleRate_, sampleRate_, channelCount_, RESAMPLER_MAX_INPUT_FRAMES);
      // r8brain reports how much one full input block can expand to; size the output
      // to that so process() can never write past the end.
      const auto maxOutFrames = static_cast<size_t>(std::max(resampler_->getMaxOutLen(), 1));
      resamplerOutput_ = std::make_unique<AudioBuffer>(maxOutFrames, channelCount_, sampleRate_);
    }
  }

  if (!inputBufferPool_.allocate(maxInputBufferLength_, streamChannelCount_, streamSampleRate_)) {
    releaseProcessingResources();
    return Result<NoneType, std::string>::Err("Failed to preallocate recorder callback buffers");
  }

  auto offloaderLambda = [this](PendingCallbackFrames pending) {
    runCallbackTask(std::move(pending));
  };
  offloader_ = std::make_unique<Offloader>(RECORDER_CALLBACK_CHANNEL_CAPACITY, offloaderLambda);
  return Result<NoneType, std::string>::Ok(None);
}

void AudioRecorderCallback::releaseProcessingResources() {
  resampler_.reset();
  remixedChunk_.reset();
  resamplerOutput_.reset();

  inputBufferPool_.clear();
}

void AudioRecorderCallback::cleanup() {
  std::scoped_lock audioLock(destructionAudioGuard_);
  // join the worker
  offloader_.reset();

  if (!circularBuffer_.empty() && circularBuffer_[0]->getNumberOfAvailableFrames() > 0) {
    emitAudioData(true);
  }

  releaseProcessingResources();

  for (const auto &arr : circularBuffer_) {
    arr->zero();
  }
}

/// @brief Copies incoming audio into an owned slot and hands it to the worker thread.
/// This method is called on the audio thread.
void AudioRecorderCallback::receiveAudioData(const float *const *channels, int numFrames) {
  // Don't block the audio thread: if cleanup() holds the guard we're being destroyed, drop the
  // buffer.
  std::unique_lock<std::mutex> lock(destructionAudioGuard_, std::try_to_lock);
  if (!lock.owns_lock()) {
    return;
  }
  if (channels == nullptr || numFrames <= 0 || offloader_ == nullptr) {
    return;
  }
  if (!isInitialized_.load(std::memory_order_acquire)) {
    return;
  }

  AudioBufferLease slot = inputBufferPool_.tryAcquire();
  if (slot == nullptr) {
    return;
  }

  const auto frames = static_cast<size_t>(numFrames);
  if (frames > slot->getSize()) {
    return;
  }

  // The recorder owns `channels` only for the duration of this synchronous callback. Copy
  // into a leased buffer before handing off to the worker thread.
  for (int channel = 0; channel < streamChannelCount_; ++channel) {
    std::memcpy(slot->getChannel(channel)->begin(), channels[channel], frames * sizeof(float));
  }
  // send() cannot block here: we hold a slot from a pool of RECORDER_CALLBACK_POOL_SIZE,
  // and the channel is sized one larger, so the ring always has room while
  // any slot is in flight.
  offloader_->getSender()->send(
      PendingCallbackFrames{.buffer = std::move(slot), .numFrames = numFrames});
}

void AudioRecorderCallback::pushChannels(const AudioBuffer &planarFrames, int numFrames) {
  for (int ch = 0; ch < channelCount_; ++ch) {
    circularBuffer_[ch]->push_back(*planarFrames.getChannel(ch), numFrames);
  }
}

void AudioRecorderCallback::pushChannels(const float *const *planarFrames, int numFrames) {
  for (int ch = 0; ch < channelCount_; ++ch) {
    circularBuffer_[ch]->push_back(planarFrames[ch], numFrames);
  }
}

/// @brief Worker-thread handler: resamples/remixes if needed, pushes into the circular
/// buffer and emits to JS once a full callback buffer has accumulated.
void AudioRecorderCallback::runCallbackTask(PendingCallbackFrames pending) {
  // The TaskOffloader destructor sends a default-constructed PendingCallbackFrames (no
  // buffer) to unblock the receiver; ignore it here.
  if (pending.buffer == nullptr) {
    return;
  }
  const int numFrames = pending.numFrames;
  const AudioBuffer &slotFrames = *pending.buffer;

  if (resampler_ == nullptr && remixedChunk_ == nullptr) {
    // Stream already matches what JS asked for.
    pushChannels(slotFrames, numFrames);
  } else {
    convertAndPushChunks(slotFrames, numFrames);
  }

  if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
    emitAudioData();
  }
}

/// r8brain is built for a bounded input block, so the frames are fed through one chunk at a
/// time. It resamples but never remixes, so a chunk is folded to the callback's channel count
/// first when the counts differ; otherwise it is a window straight into the slot.
void AudioRecorderCallback::convertAndPushChunks(const AudioBuffer &slotFrames, int numFrames) {
  std::array<float *, MAX_CHANNEL_COUNT> resampled{};
  if (resampler_ != nullptr) {
    for (int ch = 0; ch < channelCount_; ++ch) {
      resampled[ch] = resamplerOutput_->getChannel(ch)->begin();
    }
  }

  for (int consumed = 0; consumed < numFrames;) {
    const int chunkFrames = std::min(numFrames - consumed, RESAMPLER_MAX_INPUT_FRAMES);

    std::array<const float *, MAX_CHANNEL_COUNT> chunk{};
    if (remixedChunk_ != nullptr) {
      remixedChunk_->copy(
          slotFrames, static_cast<size_t>(consumed), 0, static_cast<size_t>(chunkFrames));
      for (int ch = 0; ch < channelCount_; ++ch) {
        chunk[ch] = remixedChunk_->getChannel(ch)->begin();
      }
    } else {
      for (int ch = 0; ch < channelCount_; ++ch) {
        chunk[ch] = slotFrames.getChannel(ch)->begin() + consumed;
      }
    }

    if (resampler_ == nullptr) {
      pushChannels(chunk.data(), chunkFrames);
    } else {
      const int producedFrames = resampler_->process(chunk.data(), chunkFrames, resampled.data());
      if (producedFrames > 0) {
        pushChannels(resampled.data(), producedFrames);
      }
    }

    consumed += chunkFrames;
  }
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

/// @brief Invokes the error callback with the provided message.
/// @param message The error message to be sent to the callback.
void AudioRecorderCallback::invokeOnErrorCallback(const std::string &message) {
  errorEvent_.dispatch(StringPayload{.name = "message", .reason = message});
}

void AudioRecorderCallback::assignOnErrorCallbackId(uint64_t callbackId) {
  errorEvent_.assignCallbackId(callbackId);
}

} // namespace audioapi
