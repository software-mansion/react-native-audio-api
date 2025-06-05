#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>

#include <memory>
#include <cstddef>
#include <algorithm>
#include <string>
#include <queue>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferQueueSourceNode : public AudioScheduledSourceNode {
 public:
    explicit AudioBufferQueueSourceNode(BaseAudioContext *context);
    ~AudioBufferQueueSourceNode() override;

    [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
    [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

    void start(double when) override;
    void start(double when, double offset);
    void pause();

    void enqueueBuffer(const std::shared_ptr<AudioBuffer> &buffer, int bufferId, bool isLastBuffer);
    void disable() override;

    void setOnPositionChangedCallbackId(uint64_t callbackId);
    void setOnPositionChangedInterval(int interval);
    void sendOnPositionChangedEvent();

 protected:
    std::mutex &getBufferLock();
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
    double getStopTime() const override;

 private:
    std::mutex bufferLock_;

    // pitch correction
    std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
    std::shared_ptr<AudioBus> playbackRateBus_;

    // k-rate params
    std::shared_ptr<AudioParam> detuneParam_;
    std::shared_ptr<AudioParam> playbackRateParam_;

    // internal helper
    double vReadIndex_;

    // User provided buffers
    std::queue<std::pair<int, std::shared_ptr<AudioBuffer>>> buffers_;
    int bufferId_ = 0;
    bool isLastBuffer_ = false;
    bool isPaused_ = false;

    // positionChanged event props: callbackId, update interval in frames, time since last update in frames
    uint64_t onPositionChangedCallbackId_ = 0;
    int onPositionChangedInterval_;
    int onPositionChangedTime_ = 0;
    size_t position_ = 0;

    void processWithPitchCorrection(const std::shared_ptr<AudioBus> &processingBus,
                                    int framesToProcess);

    void processWithoutInterpolation(
            const std::shared_ptr<AudioBus>& processingBus,
            size_t startOffset,
            size_t offsetLength);
};

} // namespace audioapi
