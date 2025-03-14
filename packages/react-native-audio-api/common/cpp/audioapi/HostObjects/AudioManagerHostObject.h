#pragma once

#include <audioapi/system/AudioManager.h>
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
  }

  ~AudioManagerHostObject() override {
    AudioManager::destroyInstance();
  }

 protected:
  AudioManager *audioManager_;
};

} // namespace audioapi
