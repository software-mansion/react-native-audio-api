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
    void processAudio() override;

 private:
  std::shared_ptr<AudioParam> gainParam_;
};

} // namespace audioapi
