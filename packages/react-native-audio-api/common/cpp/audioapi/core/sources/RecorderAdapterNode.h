#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/core/BaseAudioContext.h>
#include <memory>

namespace audioapi {

class AudioBus;

/// @brief RecorderAdapterNode is an AudioNode which adapts push Recorder into pull graph.
/// It uses RingBuffer to store audio data and AudioParam to provide audio data in pull mode.
/// It is used to connect native audio recording APIs with Audio API.
///
/// @note it will push silence if it is not connected to any Recorder
class RecorderAdapterNode : public AudioNode{
 public:
    explicit RecorderAdapterNode(BaseAudioContext *context);
    void setRecorder(const std::shared_ptr<AudioRecorder> &recorder);

 protected:
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
 private:
    std::shared_ptr<AudioRecorder> recorder_;
};

} // namespace audioapi
