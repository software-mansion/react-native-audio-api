#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

namespace audioapi {

/// @brief RecorderAdapterNode is an AudioNode which adapts push Recorder into pull graph.
/// It uses RingBuffer to store audio data and AudioParam to provide audio data in pull mode.
/// It is used to connect native audio recording APIs with Audio API.
///
/// @note it will push silence if it is not connected to any Recorder
class RecorderAdapterNode : public AudioNode {
 public:
  explicit RecorderAdapterNode(const std::shared_ptr<BaseAudioContext> &context);

  /// @brief Sizes the ring buffers for the recorder's stream and, when its rate differs from
  /// the context's, the resampler.
  /// @note A no-op on an initialized node; adapterCleanup() first to take a new format.
  void init(const StreamFormat &streamFormat);
  void adapterCleanup();

  /// Recorder input thread, under the recorder's adapter lock. @p channels holds one pointer per
  /// channel the node was initialized with. A burst larger than the init buffer size is dropped.
  void writeFrames(const float *const *channels, size_t numFrames);

  // TODO: CircularOverflowableAudioBuffer
  std::vector<std::shared_ptr<CircularOverflowableAudioArray>> buff_;

 protected:
  void processNode(int framesToProcess) override;
  std::shared_ptr<AudioBuffer> adapterOutputBuffer_;

 private:
  void readFrames(AudioBuffer &target, size_t framesToRead);
  void processResampled(int framesToProcess);
  void waitForProcessQuiescence() const;

  std::unique_ptr<r8b::MultiChannelResampler> resampler_;
  bool needsResampling_ = false;

  // Number of input frames (at recorder rate) to feed per render quantum
  size_t inputChunkSize_ = 0;
  AudioBuffer resamplerInputBuffer_;
  AudioBuffer resamplerOutputBuffer_;

  // Accumulates resampled output across calls
  AudioBuffer overflowBuffer_;
  size_t overflowSize_ = 0;

  std::atomic<bool> isInitialized_{false};
  /// Incremented around each processNode() call; adapterCleanup waits for quiescence.
  std::atomic<uint32_t> currentProcesses_{0};
};

} // namespace audioapi
