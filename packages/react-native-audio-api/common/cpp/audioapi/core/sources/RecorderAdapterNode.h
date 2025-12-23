#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <memory>
#include <vector>

namespace audioapi {

class AudioBus;

/// @brief RecorderAdapterNode is an AudioNode which adapts push Recorder into pull graph.
/// It uses RingBuffer to store audio data and AudioParam to provide audio data in pull mode.
/// It is used to connect native audio recording APIs with Audio API.
///
/// @note it will push silence if it is not connected to any Recorder
class RecorderAdapterNode : public AudioNode {
 public:
  explicit RecorderAdapterNode(std::shared_ptr<BaseAudioContext> context);

  /// @brief Initialize the RecorderAdapterNode with a buffer size and channel count.
  /// @note This method should be called ONLY ONCE when the buffer size is known.
  /// @param bufferSize The size of the buffer to be used.
  /// @param channelCount The number of channels.
  void init(size_t bufferSize, int channelCount);
  void cleanup();

  int channelCount_;
  // TODO: CircularOverflowableAudioBus
  std::vector<std::shared_ptr<CircularOverflowableAudioArray>> buff_;

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;
  std::shared_ptr<AudioBus> adapterOutputBus_;

 private:
  /// @brief Read audio frames from the recorder's internal circular buffer into output buss.
  /// @note If `framesToRead` is greater than the number of available frames, it will fill empty space with silence.
  /// @param framesToRead Number of frames to read.
  void readFrames(size_t framesToRead);
};

} // namespace audioapi
