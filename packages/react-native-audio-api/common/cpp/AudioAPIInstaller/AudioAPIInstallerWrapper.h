#pragma once

#include <memory>
#include <string>
#include <utility>

#include "AudioContextWrapper.h"

#ifdef ANDROID
#include "AudioAPIInstaller.h"
#endif

namespace audioapi {

#ifdef ANDROID
class AudioAPIInstaller;
#endif

class AudioAPIInstallerWrapper {
#ifdef ANDROID

 private:
  AudioAPIInstaller *audioAPIInstaller_;

 public:
  explicit AudioAPIInstallerWrapper(AudioAPIInstaller *audioAPIInstaller)
      : audioAPIInstaller_(audioAPIInstaller) {}
#else

 public:
  AudioAPIInstallerWrapper() {}
#endif
 public:
  std::shared_ptr<AudioContextWrapper> createAudioContext();
};
} // namespace audioapi
