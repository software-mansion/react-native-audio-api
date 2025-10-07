#pragma once

#include <audioapi/core/inputs/AudioRecorder.h>

#include <oboe/Oboe.h>
#include <functional>
#include <memory>

#include <audioapi/android/core/NativeAudioRecorder.hpp>

namespace audioapi {

using namespace oboe;

class AudioBus;

class AndroidAudioRecorder : public AudioStreamDataCallback, public AudioRecorder {
 public:
    AndroidAudioRecorder(float sampleRate,
                         int bufferLength,
                         const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry
                        );

    ~AndroidAudioRecorder() override;

    void start() override;
    void stop() override;

    DataCallbackResult onAudioReady(
            AudioStream *oboeStream,
            void *audioData,
            int32_t numFrames) override;

 private:
    std::shared_ptr<AudioStream> mStream_;

    facebook::jni::global_ref<NativeAudioRecorder> nativeAudioRecorder_;
};

} // namespace audioapi
