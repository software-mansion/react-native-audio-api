#pragma once

#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/graph/HostGraph.hpp>
#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {
using namespace facebook;

/// HostObject for AudiodestinationNode, which is the end point of the audio graph.
/// It is treated differently than other AudioNodes, because it shares ownership of underlying AudioDestinationNode with BaseAudioContext.
/// Hence all AudioNodeHostObject methods and proprties has to be implemented duplicated.
class AudioDestinationNodeHostObject : public JsiHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      utils::graph::HostGraph::Node *node,
      std::shared_ptr<AudioDestinationNode> destination,
      const AudioDestinationOptions &options = AudioDestinationOptions());

  [[nodiscard]] utils::graph::HostGraph::Node *rawNode() const {
    return node_;
  }

  JSI_PROPERTY_GETTER_DECL(numberOfInputs);
  JSI_PROPERTY_GETTER_DECL(numberOfOutputs);
  JSI_PROPERTY_GETTER_DECL(channelCount);
  JSI_PROPERTY_GETTER_DECL(channelCountMode);
  JSI_PROPERTY_GETTER_DECL(channelInterpretation);

  JSI_HOST_FUNCTION_DECL(connect);
  JSI_HOST_FUNCTION_DECL(disconnect);

 private:
  // borrowed from BaseAudioContext's graph - non-owning
  // no risk of dangling pointer here since destination node in TS holds ref to context =>
  // destination HO will not outlive context HO => graph is owned by context HO
  utils::graph::HostGraph::Node *node_; // borrowed from BaseAudioContext's graph - non-owning
  std::shared_ptr<AudioDestinationNode> destination_;

  const int numberOfInputs_;
  const int numberOfOutputs_;
  size_t channelCount_;
  const ChannelCountMode channelCountMode_;
  const ChannelInterpretation channelInterpretation_;
};

} // namespace audioapi
