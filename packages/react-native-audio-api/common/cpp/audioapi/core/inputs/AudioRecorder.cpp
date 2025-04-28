#ifdef ANDROID
#include <audioapi/android/core/AndroidAudioRecorder.h>
#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AudioRecorder::AudioRecorder(
    float sampleRate,
    int bufferLength,
    const std::function<void(void)> &onError,
    const std::function<void(void)> &onStatusChange,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)>
        &onAudioReady)
    : onError_(onError),
      onStatusChange_(onStatusChange),
      onAudioReady_(onAudioReady) {
#ifdef ANDROID
  audioRecorder_ = std::make_shared<AndroidAudioRecorder>(
      sampleRate, bufferLength, this->getOnAudioReady());
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>(
      sampleRate, bufferLength, this->getOnAudioReady());
#endif
}

AudioRecorder::~AudioRecorder() {
  onError_ = nullptr;
  onStatusChange_ = nullptr;
  onAudioReady_ = nullptr;
}

void AudioRecorder::start() {
  audioRecorder_->start();
}

void AudioRecorder::stop() {
  audioRecorder_->stop();
}

std::function<void(std::shared_ptr<AudioBus>, int, double)>
AudioRecorder::getOnAudioReady() {
  return
      [this](const std::shared_ptr<AudioBus> &bus, int numFrames, double when) {
        onAudioReady_(bus, numFrames, when);
      };
}

} // namespace audioapi
