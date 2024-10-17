#pragma once

#include <oboe/Oboe.h>
#include <memory>

namespace audioapi {

    using namespace oboe;

    class AudioContext;

    class AudioPlayer : public AudioStreamDataCallback {
    public:
        explicit AudioPlayer(AudioContext *context);

        int getSampleRate() const;
        int getFramesPerBurst() const;
        void start();
        void stop();

        DataCallbackResult onAudioReady(
                AudioStream *oboeStream,
                void *audioData,
                int32_t numFrames) override;

    private:
        AudioContext *context_;
        std::shared_ptr<AudioStream> mStream_;
    };

} // namespace audioapi
