#pragma once

#include <memory>

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class GainNode : public AudioNode {
 public:
  explicit GainNode();

  std::shared_ptr<AudioParam> getGainParam() const;

 protected:
  void process(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames,
      int channelCount) override;

 private:
  std::shared_ptr<AudioParam> gainParam_;
};

} // namespace audioapi
