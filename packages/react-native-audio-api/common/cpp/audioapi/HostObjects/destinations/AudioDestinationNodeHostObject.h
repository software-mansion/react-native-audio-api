#pragma once

#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/graph/HostGraph.hpp>
#include <audioapi/jsi/JsiHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject : public JsiHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      utils::graph::HostGraph::Node *node,
      std::shared_ptr<AudioDestinationNode> destination);

  [[nodiscard]] utils::graph::HostGraph::Node *rawNode() const {
    return node_;
  }

  JSI_PROPERTY_GETTER_DECL(numberOfInputs);
  JSI_PROPERTY_GETTER_DECL(numberOfOutputs);
  JSI_PROPERTY_GETTER_DECL(channelCount);

 private:
  utils::graph::HostGraph::Node *node_; // borrowed from BaseAudioContext; never removed
  std::shared_ptr<AudioDestinationNode> destination_;
};

} // namespace audioapi
