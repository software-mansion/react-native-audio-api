#pragma once

#include <memory>

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(AudioContext *context);
  std::shared_ptr<AudioParam> getPanParam() const;

 protected:
    void processAudio() override;

 private:
  std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
