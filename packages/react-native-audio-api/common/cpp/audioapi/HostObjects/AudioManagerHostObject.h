#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <utility>

#include <JsiHostObject.h>
#include <JsiPromise.h>

#include "AudioManager.h"

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
