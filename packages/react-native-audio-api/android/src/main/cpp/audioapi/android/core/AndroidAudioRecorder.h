#pragma once

#include <audioapi/core/inputs/AudioRecorder.h>

#include <oboe/Oboe.h>
#include <functional>
#include <memory>

namespace audioapi {

using namespace oboe;

class AudioBus;

class AndroidAudioRecorder : public AudioStreamDataCallback, public AudioRecorder {
 public:
    AndroidAudioRecorder(float sampleRate,
                         int bufferLength,
                         const std::function<void(void)> &onError,
                         const std::function<void(void)> &onStatusChange,
                         const std::function<void(std::shared_ptr<AudioBus>, int, double)> &onAudioReady);

    ~AndroidAudioRecorder() override;

    void start() override;
    void stop() override;

    DataCallbackResult onAudioReady(
            AudioStream *oboeStream,
            void *audioData,
            int32_t numFrames) override;

 private:
    std::shared_ptr<AudioStream> mStream_;
};

} // namespace audioapi
