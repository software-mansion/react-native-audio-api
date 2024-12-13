#pragma once

#include <memory>
#include <vector>

#include "AudioDestinationNode.h"
#include "AudioNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      const std::shared_ptr<AudioDestinationNode> &node)
      : AudioNodeHostObject(node) {}
};
} // namespace audioapi
