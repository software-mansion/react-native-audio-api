#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/dsp/r8brain/Resampler.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <memory>
#include <vector>

namespace audioapi {

class AudioBuffer;

/// @brief RecorderAdapterNode is an AudioNode which adapts push Recorder into pull graph.
/// It uses RingBuffer to store audio data and AudioParam to provide audio data in pull mode.
/// It is used to connect native audio recording APIs with Audio API.
///
/// @note it will push silence if it is not connected to any Recorder
class RecorderAdapterNode : public AudioNode {
 public:
  explicit RecorderAdapterNode(const std::shared_ptr<BaseAudioContext> &context);

  /// @brief Initialize the RecorderAdapterNode with a buffer size and channel count.
  /// @note This method should be called ONLY ONCE when the buffer size is known.
  /// @param bufferSize The size of the buffer to be used.
  /// @param channelCount The number of channels.
  /// @param sampleRate The recorder's native sample rate.
  void init(size_t bufferSize, int channelCount, float sampleRate);
  void cleanup();

  // TODO: CircularOverflowableAudioBuffer
  std::vector<std::shared_ptr<CircularOverflowableAudioArray>> buff_;

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;
  std::shared_ptr<AudioBuffer> adapterOutputBuffer_;

 private:
  void readFrames(AudioBuffer &target, size_t framesToRead);
  void processResampled(int framesToProcess);

  std::unique_ptr<r8b::MultiChannelResampler> resampler_;
  bool needsResampling_ = false;

  // Number of input frames (at recorder rate) to feed per render quantum
  size_t inputChunkSize_ = 0;
  AudioBuffer resamplerInputBuffer_;
  AudioBuffer resamplerOutputBuffer_;

  // Accumulates resampled output across calls
  AudioBuffer overflowBuffer_;
  size_t overflowSize_ = 0;
};

} // namespace audioapi
