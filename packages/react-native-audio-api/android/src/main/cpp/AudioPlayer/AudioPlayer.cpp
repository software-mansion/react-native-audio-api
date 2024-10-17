#include "AudioPlayer.h"
#include "AudioContext.h"

namespace audioapi {
    AudioPlayer::AudioPlayer(AudioContext *context): context_(context) {
        AudioStreamBuilder builder;
        builder.setSharingMode(SharingMode::Exclusive)
                ->setFormat(AudioFormat::Float)
                ->setFormatConversionAllowed(true)
                ->setPerformanceMode(PerformanceMode::LowLatency)
                ->setChannelCount(CHANNEL_COUNT)
                ->setSampleRateConversionQuality(SampleRateConversionQuality::Medium)
                ->setDataCallback(this)
                ->openStream(mStream_);
    }

    int AudioPlayer::getSampleRate() const {
        return mStream_->getSampleRate();
    }

    int AudioPlayer::getFramesPerBurst() const {
        return mStream_->getFramesPerBurst();
    }

    void AudioPlayer::start() {
        if (mStream_) {
            mStream_->requestStart();
        }
    }

    void AudioPlayer::stop() {
        if (mStream_) {
            mStream_->requestStop();
            mStream_->close();
            mStream_.reset();
        }
    }

    DataCallbackResult AudioPlayer::onAudioReady(
            AudioStream *oboeStream,
            void *audioData,
            int32_t numFrames) {
        auto buffer = static_cast<float *>(audioData);
        context_->renderAudio(buffer, numFrames);

        return DataCallbackResult::Continue;
    }
}
