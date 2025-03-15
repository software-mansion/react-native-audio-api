#pragma once

#include <audioapi/system/AudioManager.h>
#include <audioapi/system/SessionOptions.h>
#include <jsi/jsi.h>

#include <utility>
#include <memory>

#include <JsiHostObject.h>
#include <JsiPromise.h>


namespace audioapi {
using namespace facebook;

class AudioManagerHostObject : public JsiHostObject {
 public:
  explicit AudioManagerHostObject() {
    audioManager_ = AudioManager::getInstance();

    addFunctions(
      JSI_EXPORT_FUNCTION(AudioManagerHostObject, setOptions));
  }

  ~AudioManagerHostObject() override {
    AudioManager::destroyInstance();
  }

  JSI_HOST_FUNCTION(setOptions) {
    auto options = SessionOptions::fromJSIValue(args[0], runtime);
    audioManager_->setSessionOptions(options);

    return jsi::Value::undefined();
  }

 protected:
  AudioManager *audioManager_;
};

} // namespace audioapi
