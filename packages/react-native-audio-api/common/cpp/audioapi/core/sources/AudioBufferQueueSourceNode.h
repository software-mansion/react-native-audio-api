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

    void stop(double when) override;
    void pause();

    void enqueueBuffer(const std::shared_ptr<AudioBuffer> &buffer, bool isLastBuffer);
    void dequeueBuffer(const std::shared_ptr<AudioBuffer> &buffer);
    void clearBuffers();

    void disable() override;

 protected:
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
    double getCurrentPosition() const override;

 private:
    // User provided buffers
    std::queue<std::shared_ptr<AudioBuffer>> buffers_;
    bool isLastBuffer_ = false;
    bool isPaused_ = false;

    double playedBuffersDuration_ = 0;

    void processWithoutInterpolation(
            const std::shared_ptr<AudioBus>& processingBus,
            size_t startOffset,
            size_t offsetLength,
            float playbackRate) override;
};

} // namespace audioapi
