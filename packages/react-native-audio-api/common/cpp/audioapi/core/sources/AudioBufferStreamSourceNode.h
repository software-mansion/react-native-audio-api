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

class AudioBufferStreamSourceNode : public AudioScheduledSourceNode {
 public:
    explicit AudioBufferStreamSourceNode(BaseAudioContext *context);
    ~AudioBufferStreamSourceNode() override;

    [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
    [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;

    void start(double when, double offset);
    void enqueueAudioBuffer(const std::shared_ptr<AudioBuffer> &buffer);
    void disable() override;

 protected:
    std::mutex &getBufferLock();
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
    double getStopTime() const override;

 private:
    std::mutex bufferLock_;

    // pitch correction
    std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;

    // k-rate params
    std::shared_ptr<AudioParam> detuneParam_;
    std::shared_ptr<AudioParam> playbackRateParam_;

    std::shared_ptr<AudioBus> playbackRateBus_;

    // internal helper
    double vReadIndex_;

    // User provided buffer
    std::queue<std::shared_ptr<AudioBuffer>> buffers_;

    void processWithPitchCorrection(const std::shared_ptr<AudioBus> &processingBus,
                                    int framesToProcess);

    void processWithoutInterpolation(
            const std::shared_ptr<AudioBus>& processingBus,
            size_t startOffset,
            size_t offsetLength);
};

} // namespace audioapi
