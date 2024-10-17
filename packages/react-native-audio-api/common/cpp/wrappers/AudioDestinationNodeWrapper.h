#pragma once

#include <memory>

#include "AudioNodeWrapper.h"
#include "AudioDestinationNode.h"

namespace audioapi {

class AudioDestinationNodeWrapper : public AudioNodeWrapper {

 public:
  explicit AudioDestinationNodeWrapper(
      const std::shared_ptr<AudioDestinationNode> &destinationNode)
      : AudioNodeWrapper(destinationNode) {}
};
} // namespace audioapi
