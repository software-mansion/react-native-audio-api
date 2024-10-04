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

public:
  explicit AudioAPIInstallerWrapper(AudioAPIInstaller *audioAPIInstaller);

private:
    AudioAPIInstaller *audioAPIInstaller_;
#else

 public:
  AudioAPIInstallerWrapper() = default;
#endif

public:
  std::shared_ptr<AudioContextWrapper> createAudioContext() const;
};
} // namespace audioapi
