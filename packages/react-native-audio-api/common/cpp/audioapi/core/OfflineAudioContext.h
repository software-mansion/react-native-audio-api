#pragma once

#include "BaseAudioContext.h"

namespace audioapi {

class OfflineAudioContext : public BaseAudioContext {
 public:
  explicit OfflineAudioContext(float sampleRate, int32_t numFrames);
  ~OfflineAudioContext() override;

  void resume();
  void suspend();

  std::shared_ptr<AudioBuffer> startRendering();

 private:
  int32_t numFrames_;

};

} // namespace audioapi