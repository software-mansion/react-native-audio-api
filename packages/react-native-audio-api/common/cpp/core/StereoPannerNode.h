#pragma once

#include <algorithm>
#include <memory>

#include "AudioNode.h"
#include "AudioParam.h"
#include "VectorMath.h"

namespace audioapi {

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(BaseAudioContext *context);

  [[nodiscard]] std::shared_ptr<AudioParam> getPanParam() const;

 protected:
  bool processAudio(float *audioData, int32_t numFrames) override;

 private:
  std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
