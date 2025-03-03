#pragma once

#include <audioapi/libs/signalsmith-stretch/signalsmith-stretch.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>

#include <memory>
#include <string>
#include <cassert>

namespace audioapi {
class AudioBus;

class StretcherNode : public AudioNode {
 public:
    explicit StretcherNode(BaseAudioContext *context);

    [[nodiscard]] std::shared_ptr<AudioParam> getRateParam() const;
    [[nodiscard]] std::shared_ptr<AudioParam> getSemitonesParam() const;

 protected:
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
    std::shared_ptr<AudioBus> processAudio(std::shared_ptr<AudioBus> outputBus, int framesToProcess, bool checkIsAlreadyProcessed) override;

 private:
    // k-rate params
    std::shared_ptr<AudioParam> rate_;
    std::shared_ptr<AudioParam> semitones_;

    std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
    std::shared_ptr<AudioBus> playbackRateBus_;
    int framesNeededToStretch_ = RENDER_QUANTUM_SIZE;
};

} // namespace audioapi
