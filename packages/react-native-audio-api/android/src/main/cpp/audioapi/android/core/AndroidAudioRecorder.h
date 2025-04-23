#pragma once

#include <oboe/Oboe.h>
#include <functional>
#include <memory>

namespace audioapi {

using namespace oboe;

class AudioBus;

class AndroidAudioRecorder : public AudioStreamDataCallback {
 public:
    AndroidAudioRecorder(float sampleRate,
                         int bufferLength,
                         const std::function<void(std::shared_ptr<AudioBus>, int, double)>
                         &onAudioReady);

    ~AndroidAudioRecorder() override;

    void start();
    void stop();

    DataCallbackResult onAudioReady(
            AudioStream *oboeStream,
            void *audioData,
            int32_t numFrames) override;

 private:
    float sampleRate_;
    int bufferLength_;
    std::shared_ptr<AudioStream> mStream_;
    std::function<void(std::shared_ptr<AudioBus>, int, double)> onAudioReady_;
};

} // namespace audioapi
