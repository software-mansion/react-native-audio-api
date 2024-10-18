#pragma once

#include <memory>

#include "AudioDestinationNode.h"
#include "AudioNodeWrapper.h"

namespace audioapi {

class AudioDestinationNodeWrapper : public AudioNodeWrapper {
 public:
  explicit AudioDestinationNodeWrapper(
      const std::shared_ptr<AudioDestinationNode> &destinationNode)
      : AudioNodeWrapper(destinationNode) {}
};
} // namespace audioapi
