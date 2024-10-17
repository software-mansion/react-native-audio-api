#pragma once

#include <memory>

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class GainNode : public AudioNode {
 public:
  explicit GainNode(AudioContext *context);

  std::shared_ptr<AudioParam> getGainParam() const;

 protected:
  bool processAudio(float *audioData, int32_t numFrames) override;

 private:
  std::shared_ptr<AudioParam> gainParam_;
};

} // namespace audioapi
