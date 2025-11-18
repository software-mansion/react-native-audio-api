#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioDestinationNodeHostObject(const std::shared_ptr<AudioDestinationNode> &node)
      : AudioNodeHostObject(node) {}
};
} // namespace audioapi
