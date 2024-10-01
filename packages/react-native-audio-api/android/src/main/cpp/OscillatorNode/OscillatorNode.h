#pragma once

#include <cmath>
#include <string>
#include "AudioParam.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

using namespace oboe;

class OscillatorNode : public AudioStreamDataCallback,
                       public AudioScheduledSourceNode {
 public:
  explicit OscillatorNode();

  DataCallbackResult onAudioReady(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames) override;

 private:
  float gain_ = 0.5f;
  float frequency_ = 440;
  float detune_ = 0.0;
  float phase_ = 0.0;
};
} // namespace audioapi
