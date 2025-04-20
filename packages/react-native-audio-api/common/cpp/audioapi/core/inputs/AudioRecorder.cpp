#ifdef ANDROID

#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

namespace audioapi {

AudioRecorder::AudioRecorder(
    std::function<void(void)> onError,
    std::function<void(void)> onStatusChange,
    std::function<void(std::shared_ptr<AudioBus>, int, double)> onAudioReady)
    : onError_(onError),
      onStatusChange_(onStatusChange),
      onAudioReady_(onAudioReady) {
#ifdef ANDROID
  // Android-specific initialization
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>(this->getOnAudioReady());
#endif
}

AudioRecorder::~AudioRecorder() {}

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
        // TODO: potentialy push data to connected graph
        onAudioReady_(bus, numFrames, when);
      };
}

} // namespace audioapi
