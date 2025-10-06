#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/utils/CircularOverflowableAudioArray.h>
#include <memory>

namespace audioapi {

class AudioBus;

/// @brief RecorderAdapterNode is an AudioNode which adapts push Recorder into pull graph.
/// It uses RingBuffer to store audio data and AudioParam to provide audio data in pull mode.
/// It is used to connect native audio recording APIs with Audio API.
///
/// @note it will push silence if it is not connected to any Recorder
class RecorderAdapterNode : public AudioNode {
 public:
    explicit RecorderAdapterNode(BaseAudioContext *context);

    /// @brief Initialize the RecorderAdapterNode with a buffer size.
    /// @note This method should be called ONLY ONCE when the buffer size is known.
    /// @throws std::runtime_error if the node is already initialized.
    /// @param bufferSize The size of the buffer to be used.
    void init(size_t bufferSize);

 protected:
    std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
    std::shared_ptr<CircularOverflowableAudioArray> buff_;

 private:
    /// @brief Read audio frames from the recorder's internal adapterBuffer.
    /// @note If `framesToRead` is greater than the number of available frames, it will fill empty space with silence.
    /// @param output Pointer to the output buffer.
    /// @param framesToRead Number of frames to read.
    void readFrames(float *output, size_t framesToRead);

    friend class AudioRecorder;
};

} // namespace audioapi
