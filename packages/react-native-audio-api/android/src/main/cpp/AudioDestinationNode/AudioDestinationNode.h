#pragma once

#include <algorithm>
#include <vector>

#include "AudioNode.h"

namespace audioapi {

class AudioDestinationNode : public AudioNode {
public:
  explicit AudioDestinationNode(AudioContext *context);

  const std::vector<float> &getOutputBuffer();

protected:
    void processAudio() override;
};

} // namespace audioapi
