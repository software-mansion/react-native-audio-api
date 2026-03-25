#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      const std::shared_ptr<utils::graph::Graph> &graph,
      std::unique_ptr<AudioNode> node)
      : AudioNodeHostObject(graph, std::move(node), AudioDestinationOptions()) {}
};
} // namespace audioapi
