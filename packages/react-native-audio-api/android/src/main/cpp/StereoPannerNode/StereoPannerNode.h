#pragma once

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode();
  std::shared_ptr<AudioParam> getPanParam() const;

 protected:
  void process(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames,
      int channelCount) override;

 private:
  std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
