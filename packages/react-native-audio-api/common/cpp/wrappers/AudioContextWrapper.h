#pragma once

#include <memory>

#include "AudioContext.h"
#include "BaseAudioContextWrapper.h"

namespace audioapi {

class AudioContextWrapper : public BaseAudioContextWrapper {
 public:
  explicit AudioContextWrapper(const std::shared_ptr<AudioContext> &context);

  void close();

 private:
  std::shared_ptr<AudioContext> getAudioContextFromBaseAudioContext();
};
} // namespace audioapi
