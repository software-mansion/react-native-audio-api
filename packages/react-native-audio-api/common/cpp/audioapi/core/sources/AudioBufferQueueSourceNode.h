#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioBufferBaseSourceNode.h>
#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>

#include <memory>
#include <cstddef>
#include <algorithm>
#include <string>
#include <queue>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferQueueSourceNode : public AudioBufferBaseSourceNode {
 public:
    explicit AudioBufferQueueSourceNode(BaseAudioContext *context);
    ~AudioBufferQueueSourceNode() override;

    [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
    [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

    void start(double when, double offset);
    void stop(double when) override;
    void pause();

    void enqueueBuffer(const std::shared_ptr<AudioBuffer> &buffer, int bufferId, bool isLastBuffer);
    void disable() override;

 protected:
    std::mutex &getBufferLock();
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
    double getCurrentPosition() const override;

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

    double position_ = 0;

    void processWithPitchCorrection(const std::shared_ptr<AudioBus> &processingBus,
                                    int framesToProcess);

    void processWithoutInterpolation(
            const std::shared_ptr<AudioBus>& processingBus,
            size_t startOffset,
            size_t offsetLength);
};

} // namespace audioapi
