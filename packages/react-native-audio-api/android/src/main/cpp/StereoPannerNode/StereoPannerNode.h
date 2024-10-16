#pragma once

#include <algorithm>
#include <memory>

#include "AudioNode.h"
#include "AudioParam.h"
#include "VectorMath.h"

namespace audioapi {

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(AudioContext *context);
  std::shared_ptr<AudioParam> getPanParam() const;

 protected:
  bool processAudio(float *audioData, int32_t numFrames) override;

 private:
  std::shared_ptr<AudioParam> panParam_;

  void normalize(float *audioData, int32_t numFrames);
};

} // namespace audioapi
