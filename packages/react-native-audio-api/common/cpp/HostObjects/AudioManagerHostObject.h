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
  explicit AudioManagerHostObject(const std::shared_ptr<AudioManager> &audioManager): audioManager_(audioManager) {}

 protected:
  std::shared_ptr<AudioManager> audioManager_;
};

} // namespace audioapi
