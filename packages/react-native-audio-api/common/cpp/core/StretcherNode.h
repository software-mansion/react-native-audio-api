#pragma once

#include <memory>
#include <string>

#include "AudioNode.h"
#include "signalsmith-stretch.h"
#include "TimeStretchType.h"
//#include "AudioParam.h"

namespace audioapi {
class AudioBus;

class StretcherNode : public AudioNode {
 public:
    explicit StretcherNode(BaseAudioContext *context);

    [[nodiscard]] std::string getTimeStretch() const;

    void setTimeStretch(const std::string& timeStretchType);

 protected:
    void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

private:
    TimeStretchType timeStretch_ = TimeStretchType::LINEAR;

    std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
    std::shared_ptr<AudioBus> playbackRateBus_;

    static TimeStretchType fromString(const std::string &type) {
        std::string lowerType = type;
        std::transform(
                lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

        if (lowerType == "linear")
            return TimeStretchType::LINEAR;
        if (lowerType == "speech")
            return TimeStretchType::SPEECH;
        if (lowerType == "music")
            return TimeStretchType::MUSIC;

        throw std::invalid_argument("Unknown time stretch type: " + type);
    }

    static std::string toString(TimeStretchType type) {
        switch (type) {
            case TimeStretchType::LINEAR:
                return "linear";
            case TimeStretchType::SPEECH:
                return "speech";
            case TimeStretchType::MUSIC:
                return "music";
            default:
                throw std::invalid_argument("Unknown time stretch type");
        }
    }
};

} // namespace audioapi
