#pragma once

#include <audioapi/system/AudioManager.h>
#include <audioapi/system/SessionOptions.h>
#include <audioapi/system/LockScreenInfo.h>
#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/jsi/JsiPromise.h>
#include <jsi/jsi.h>

#include <utility>
#include <memory>


namespace audioapi {
using namespace facebook;

class AudioManagerHostObject : public JsiHostObject {
 public:
  explicit AudioManagerHostObject() {
    audioManager_ = AudioManager::getInstance();

    addFunctions(
      JSI_EXPORT_FUNCTION(AudioManagerHostObject, setOptions),
      JSI_EXPORT_FUNCTION(AudioManagerHostObject, setNowPlaying));
  }

  ~AudioManagerHostObject() override {
    AudioManager::destroyInstance();
  }

  JSI_HOST_FUNCTION(setOptions) {
    auto options = SessionOptions::fromJSIValue(args[0], runtime);
    audioManager_->setSessionOptions(options);

    return jsi::Value::undefined();
  }


  JSI_HOST_FUNCTION(setNowPlaying) {
    auto lockScreenInfo = LockScreenInfo::fromJSIValue(args[0], runtime);
    audioManager_->setNowPlaying(lockScreenInfo);

    return jsi::Value::undefined();
  }

 protected:
  AudioManager *audioManager_;
};

} // namespace audioapi
