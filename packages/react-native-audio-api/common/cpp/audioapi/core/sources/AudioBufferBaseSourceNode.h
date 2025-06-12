#pragma once

#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>

#include <memory>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferBaseSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferBaseSourceNode(BaseAudioContext *context);

    [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
    [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

    void setOnPositionChangedCallbackId(uint64_t callbackId);
    void setOnPositionChangedInterval(int interval);

 protected:
    std::mutex bufferLock_;

    // pitch correction
    std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
    std::shared_ptr<AudioBus> playbackRateBus_;

    // k-rate params
    std::shared_ptr<AudioParam> detuneParam_;
    std::shared_ptr<AudioParam> playbackRateParam_;

    // internal helper
    double vReadIndex_;

    uint64_t onPositionChangedCallbackId_ = 0;
    int onPositionChangedInterval_;
    int onPositionChangedTime_ = 0;

    std::mutex &getBufferLock();
    virtual double getCurrentPosition() const = 0;

    void sendOnPositionChangedEvent();
    void processWithPitchCorrection(const std::shared_ptr<AudioBus> &processingBus,
                                    int framesToProcess);
    virtual void processWithoutInterpolation(
            const std::shared_ptr<AudioBus>& processingBus,
            size_t startOffset,
            size_t offsetLength,
            float playbackRate) = 0;
};

} // namespace audioapi
