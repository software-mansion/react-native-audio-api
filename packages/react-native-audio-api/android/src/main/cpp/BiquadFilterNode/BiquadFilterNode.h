#pragma once

#include <string>
#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

// TODO: Implement this class

class BiquadFilterNode : public AudioNode {
 public:
  explicit BiquadFilterNode(AudioContext *context);
};

} // namespace audioapi
