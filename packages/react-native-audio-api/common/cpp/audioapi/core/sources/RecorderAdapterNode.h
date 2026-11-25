#pragma once

#include <audioapi/core/AudioConsumer.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/dsp/r8brain/Resampler.hpp>
#include <audioapi/encoding/StreamFormat.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace audioapi {

/// @brief The recorder's end of a RecorderAdapterNode: adapts the recorder's push stream into the
/// graph's pull. The recorder writes its frames into ring buffers here, and the node renders them,
/// resampled to the context's rate.
///
/// The node and the recorder share only this consumer, so the recorder never keeps the node
/// alive; once the node is gone, its frames reach no one.
class RecorderAdapterConsumer final : public AudioConsumer {
 public:
  /// Renders at @p contextSampleRate, the node's context's rate, which never changes.
  explicit RecorderAdapterConsumer(float contextSampleRate);

  /// @brief Sizes the ring buffers for the recorder's stream and, when its rate differs from
  /// the context's, the resampler.
  Result<NoneType, std::string> prepare(const StreamFormat &streamFormat) override;
  /// Appends the frames to the ring buffers the render thread reads from; a burst larger than the
  /// prepared buffer size is dropped.
  Result<NoneType, std::string> consume(const AudioBuffer &frames, size_t numFrames) override;
  /// Waits for a render quantum in flight, then frees what prepare() sized; the node
  /// renders silence until prepared again.
  void close() override;

  /// Audio thread: sums the next @p framesToProcess frames into @p output, or zeroes it while not
  /// prepared.
  void render(DSPAudioBuffer &output, int framesToProcess);

  /// Channels of the last prepared format; 2 before the first.
  [[nodiscard]] size_t outputChannelCount() const {
    return static_cast<size_t>(outputChannelNumber_.load(std::memory_order_acquire));
  }

  /// Channels the ring buffers are sized for, or 0 while closed.
  [[nodiscard]] size_t preparedChannelCount() const {
    return buff_.size();
  }

 private:
  void readFrames(AudioBuffer &target, size_t framesToRead);
  void processResampled(int framesToProcess);
  void waitForProcessQuiescence() const;

  const float contextSampleRate_;
  std::atomic<int> outputChannelNumber_{2};

  // TODO: CircularOverflowableAudioBuffer
  std::vector<std::shared_ptr<CircularOverflowableAudioArray>> buff_;
  std::shared_ptr<AudioBuffer> adapterOutputBuffer_;

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
  /// Incremented around each render() call; close() waits for quiescence.
  std::atomic<uint32_t> currentProcesses_{0};
};

/// @brief Plays what an AudioRecorder captures inside an audio graph: renders what its
/// RecorderAdapterConsumer receives. Renders silence until the consumer is prepared.
class RecorderAdapterNode : public AudioNode {
 public:
  explicit RecorderAdapterNode(const std::shared_ptr<BaseAudioContext> &context);

  /// The end a recorder feeds.
  [[nodiscard]] const std::shared_ptr<RecorderAdapterConsumer> &consumer() const {
    return consumer_;
  }

  [[nodiscard]] std::optional<size_t> getOutputChannelNumber() const override;

 protected:
  void processNode(int framesToProcess) override;

 private:
  std::shared_ptr<RecorderAdapterConsumer> consumer_;
};

} // namespace audioapi
