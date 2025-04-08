#ifdef ANDROID

#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

#include <audioapi/core/inputs/AudioRecorder.h>

namespace audioapi {

AudioRecorder::AudioRecorder() {
#ifdef ANDROID
  // Android-specific initialization
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>();
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

} // namespace audioapi
