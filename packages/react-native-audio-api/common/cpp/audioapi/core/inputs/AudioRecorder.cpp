#ifdef ANDROID

#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AudioRecorder::AudioRecorder(
    float sampleRate,
    int numberOfChannels,
    int bufferLength,
    bool enableVoiceProcessing,
    const std::function<void(void)> &onError,
    const std::function<void(void)> &onStatusChange,
    const std::function<void(std::shared_ptr<AudioBus>, int, double)>
        &onAudioReady)
    : onError_(onError),
      onStatusChange_(onStatusChange),
      onAudioReady_(onAudioReady) {
#ifdef ANDROID
  // Android-specific initialization
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>(
      sampleRate,
      numberOfChannels,
      bufferLength,
      enableVoiceProcessing,
      this->getOnAudioReady());
#endif
}

AudioRecorder::~AudioRecorder() {
  onError_ = nullptr;
  onStatusChange_ = nullptr;
  onAudioReady_ = nullptr;
}

void AudioRecorder::start() {
#ifdef ANDROID
  // Android-specific start logic
#else
  audioRecorder_->start();
#endif
}

void AudioRecorder::stop() {
#ifdef ANDROID
  // Android-specific stop logic
#else
  audioRecorder_->stop();
#endif
}

std::function<void(std::shared_ptr<AudioBus>, int, double)>
AudioRecorder::getOnAudioReady() {
  return
      [this](const std::shared_ptr<AudioBus> &bus, int numFrames, double when) {
        // TODO: potentially push data to connected graph
        onAudioReady_(bus, numFrames, when);
      };
}

} // namespace audioapi
