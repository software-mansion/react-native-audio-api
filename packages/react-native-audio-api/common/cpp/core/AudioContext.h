#pragma once

#include <memory>

#include "BaseAudioContext.h"

namespace audioapi {

class AudioContext : public BaseAudioContext {
 public:
  AudioContext();

  void close();
};

} // namespace audioapi
