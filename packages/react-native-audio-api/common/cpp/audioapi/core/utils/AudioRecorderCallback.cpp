#include <audioapi/core/utils/AudioRecorderCallback.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/events/AudioEventPayload.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>
#include <audioapi/utils/CircularArray.hpp>

#include <algorithm>
#include <cstring>
#include <memory>
#include <stdexcept>
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
    const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
    float sampleRate,
    size_t bufferLength,
    int channelCount,
    uint64_t callbackId)
    : sampleRate_(sampleRate),
      bufferLength_(bufferLength),
      channelCount_(channelCount),
      audioReadyEvent_(audioEventHandlerRegistry) {
  audioReadyEvent_.assignCallbackId(callbackId);
  const size_t ringBufferSize =
      std::max(bufferLength * 2, static_cast<size_t>(DEFAULT_RING_BUFFER_SIZE));
  circularBuffer_.resize(channelCount_);

  for (auto &arr : circularBuffer_) {
    arr = std::make_shared<CircularAudioArray>(ringBufferSize);
  }
}

Result<NoneType, std::string> AudioRecorderCallback::prepare(const StreamFormat &streamFormat) {
  auto conversionResult = prepareConversion(streamFormat);
  if (conversionResult.is_err()) {
    releaseProcessingResources();
    return Err("Failed to prepare callback: " + conversionResult.unwrap_err());
  }
  return Ok(None);
}

Result<NoneType, std::string> AudioRecorderCallback::prepareConversion(
    const StreamFormat &streamFormat) {
  streamLayout_ = streamFormat.layout;

  if (streamLayout_.sampleRate <= 0 || streamLayout_.channelCount <= 0 ||
      streamFormat.maxFramesPerBuffer == 0) {
    return Result<NoneType, std::string>::Err("Invalid stream sample rate or channel count");
  }

  if (sampleRate_ <= 0 || channelCount_ <= 0) {
    return Result<NoneType, std::string>::Err("Invalid callback sample rate or channel count");
  }
  if (streamLayout_.channelCount > MAX_CHANNEL_COUNT || channelCount_ > MAX_CHANNEL_COUNT) {
    return Result<NoneType, std::string>::Err("Channel count exceeds MAX_CHANNEL_COUNT");
  }

  const bool needsRemix = streamLayout_.channelCount != channelCount_;
  const bool needsResampling = streamLayout_.sampleRate != sampleRate_;

  if (needsRemix || needsResampling) {
    const auto chunkFrames = static_cast<size_t>(RESAMPLER_MAX_INPUT_FRAMES);

    if (needsRemix) {
      remixedChunk_ =
          std::make_unique<AudioBuffer>(chunkFrames, channelCount_, streamLayout_.sampleRate);
    }

    if (needsResampling) {
      resampler_ = std::make_unique<r8b::MultiChannelResampler>(
          streamLayout_.sampleRate, sampleRate_, channelCount_, RESAMPLER_MAX_INPUT_FRAMES);
      // r8brain reports how much one full input block can expand to; size the output
      // to that so process() can never write past the end.
      const auto maxOutFrames = static_cast<size_t>(std::max(resampler_->getMaxOutLen(), 1));
      resamplerOutput_ = std::make_unique<AudioBuffer>(maxOutFrames, channelCount_, sampleRate_);
    }
  }

  return Result<NoneType, std::string>::Ok(None);
}

void AudioRecorderCallback::releaseProcessingResources() {
  resampler_.reset();
  remixedChunk_.reset();
  resamplerOutput_.reset();
}

void AudioRecorderCallback::invalidate() {
  audioReadyEvent_.unregisterCallback();
}

void AudioRecorderCallback::close() {
  if (!circularBuffer_.empty() && circularBuffer_[0]->getNumberOfAvailableFrames() > 0) {
    emitAudioData(true);
  }

  releaseProcessingResources();
}

Result<NoneType, std::string>
AudioRecorderCallback::pushChannels(const AudioBuffer &frames, size_t start, int numFrames) {
  try {
    for (int ch = 0; ch < channelCount_; ++ch) {
      circularBuffer_[ch]->push_back(frames.getChannel(ch)->begin() + start, numFrames);
    }
  } catch (const std::overflow_error &e) {
    return Err("Failed to push audio data into the circular buffer: " + std::string(e.what()));
  }
  return Ok(None);
}

/// Resamples/remixes if needed, pushes into the circular buffer and emits to JS once a full
/// callback buffer has accumulated.
Result<NoneType, std::string> AudioRecorderCallback::write(
    const AudioBuffer &frames,
    size_t numFrames) {
  const auto frameCount = static_cast<int>(numFrames);

  // Without conversion the stream already matches what JS asked for.
  auto pushed = resampler_ == nullptr && remixedChunk_ == nullptr
      ? pushChannels(frames, 0, frameCount)
      : convertAndPushChunks(frames, frameCount);

  // Even after a failed push: emitting is what frees the ring for the next write.
  if (circularBuffer_[0]->getNumberOfAvailableFrames() >= bufferLength_) {
    emitAudioData();
  }
  return pushed;
}

/// r8brain is built for a bounded input block, so the frames are fed through one chunk at a
/// time. It resamples but never remixes, so a chunk is folded to the callback's channel count
/// first when the counts differ; otherwise it is a window straight into the slot.
Result<NoneType, std::string> AudioRecorderCallback::convertAndPushChunks(
    const AudioBuffer &frames,
    int numFrames) {
  for (int consumed = 0; consumed < numFrames;) {
    const int chunkFrames = std::min(numFrames - consumed, RESAMPLER_MAX_INPUT_FRAMES);

    const AudioBuffer *chunk = &frames;
    auto chunkStart = static_cast<size_t>(consumed);
    if (remixedChunk_ != nullptr) {
      remixedChunk_->copy(frames, chunkStart, 0, static_cast<size_t>(chunkFrames));
      chunk = remixedChunk_.get();
      chunkStart = 0;
    }

    Result<NoneType, std::string> pushed = Ok(None);
    if (resampler_ == nullptr) {
      pushed = pushChannels(*chunk, chunkStart, chunkFrames);
    } else {
      const int producedFrames =
          resampler_->process(*chunk, chunkStart, chunkFrames, *resamplerOutput_);
      if (producedFrames > 0) {
        pushed = pushChannels(*resamplerOutput_, 0, producedFrames);
      }
    }
    if (pushed.is_err()) {
      return pushed;
    }

    consumed += chunkFrames;
  }
  return Ok(None);
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

} // namespace audioapi
