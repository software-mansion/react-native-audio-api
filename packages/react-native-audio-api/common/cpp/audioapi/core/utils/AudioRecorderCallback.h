#pragma once

#include <audioapi/core/inputs/RecorderOutput.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/events/EventCaller.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/AudioLayout.h>
#include <audioapi/utils/CircularArray.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace audioapi {

class IAudioEventHandlerRegistry;

/// Delivers recorded audio to a JS `onAudioReady` callback, resampling and remixing from
/// the input stream's format to the one JS asked for.
class AudioRecorderCallback final : public RecorderOutput {
 public:
  AudioRecorderCallback(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      size_t bufferLength,
      int channelCount,
      uint64_t callbackId);

  /// Builds the converter from @p streamFormat to the callback's format.
  Result<NoneType, std::string> prepare(const StreamFormat &streamFormat) override;
  /// Fails when the ring has no room for the frames; what fit is still emitted.
  Result<NoneType, std::string> write(const AudioBuffer &frames, size_t numFrames) override;
  /// Emits what is left, even short of a full callback buffer.
  void close() override;

  /// Unregisters the onAudioReady handler; later frames reach no JS.
  void invalidate() override;

 private:
  /// r8brain resamplers are built for a fixed maximum input block, so longer callbacks
  /// are fed through in chunks of this size.
  static constexpr int RESAMPLER_MAX_INPUT_FRAMES = 2048;

  Result<NoneType, std::string> prepareConversion(const StreamFormat &streamFormat);
  /// Remixes and/or resamples @p numFrames frames of @p frames chunk by chunk into the circular
  /// buffer.
  /// buffer. Stops at the first chunk the ring has no room for.
  Result<NoneType, std::string> convertAndPushChunks(const AudioBuffer &frames, int numFrames);
  /// Appends @p numFrames frames of @p frames, read from frame @p start, to the ring.
  Result<NoneType, std::string>
  pushChannels(const AudioBuffer &frames, size_t start, int numFrames);
  /// Emits every full callback buffer, or with @p flush everything the ring holds.
  void emitAudioData(bool flush = false);
  void invokeCallback(const std::shared_ptr<AudioBuffer> &buffer, int numFrames);
  void releaseProcessingResources();

  float sampleRate_;
  size_t bufferLength_;
  int channelCount_;
  uint64_t framesEmitted_ = 0;

  AudioLayout streamLayout_;

  EventCaller<AudioEvent::AUDIO_READY> audioReadyEvent_;

  // TODO: CircularAudioBuffer
  static constexpr size_t DEFAULT_RING_BUFFER_SIZE = 8192;
  std::vector<std::shared_ptr<CircularAudioArray>> circularBuffer_;

  /// Conversion chain, built only for the parts that are actually needed: r8brain
  /// resamples but does not remix, so a channel-count change goes through
  /// AudioBuffer::copy first. Null when the stream already matches the callback format.
  std::unique_ptr<r8b::MultiChannelResampler> resampler_;
  std::unique_ptr<AudioBuffer> remixedChunk_; // one chunk, callback channel count
  std::unique_ptr<AudioBuffer> resamplerOutput_;
};

} // namespace audioapi
