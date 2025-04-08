#pragma once

#include <memory>

namespace audioapi {
#ifdef ANDROID

#else
class IOSAudioRecorder;
#endif

class AudioRecorder {
 public:
  explicit AudioRecorder();
  ~AudioRecorder();

  void start();
  void stop();

 private:
#ifdef ANDROID

#else
  std::shared_ptr<IOSAudioRecorder> audioRecorder_;
#endif
};

} // namespace audioapi
