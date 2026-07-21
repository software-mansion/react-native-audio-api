#pragma once

#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostNode.h>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/types/NodeOptions.h>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioNode;

class AudioNodeHostObject : public HostObject,
                            public utils::graph::HostNode,
                            public std::enable_shared_from_this<AudioNodeHostObject> {
 public:
  explicit AudioNodeHostObject(
      const std::shared_ptr<utils::graph::Graph> &graph,
      std::unique_ptr<AudioNode> node,
      const AudioNodeOptions &options = AudioNodeOptions());
  ~AudioNodeHostObject() override;

  JSI_PROPERTY_GETTER_DECL(numberOfInputs);
  JSI_PROPERTY_GETTER_DECL(numberOfOutputs);
  JSI_PROPERTY_GETTER_DECL(channelCount);
  JSI_PROPERTY_GETTER_DECL(channelCountMode);
  JSI_PROPERTY_GETTER_DECL(channelInterpretation);

  JSI_PROPERTY_SETTER_DECL(channelCount);
  JSI_PROPERTY_SETTER_DECL(channelCountMode);
  JSI_PROPERTY_SETTER_DECL(channelInterpretation);

  using utils::graph::HostNode::connect;
  using utils::graph::HostNode::disconnect;

  JSI_HOST_FUNCTION_DECL(connect);
  JSI_HOST_FUNCTION_DECL(disconnect);

  [[nodiscard]] virtual size_t getMemoryPressure() const {
    return 300'000; // magic number so node can be destroyed quite fast
  }

 protected:
  /// @brief Returns the graph host node that outgoing edges for the given
  /// output index should originate from. The default returns `this` for
  /// output 0 — nodes with a single output need no override. Composite nodes
  /// (Delay, ChannelSplitter) route each output to a dedicated internal host
  /// node.
  /// @note JS thread only.
  virtual std::shared_ptr<utils::graph::HostNode> getConnectSource(int outputIndex);

  /// @brief Returns the graph host node that incoming edges for the given
  /// input index should terminate at. The default returns `this` for input 0.
  /// Composite nodes (Delay, ChannelMerger) route each input to a dedicated
  /// internal host node.
  /// @note JS thread only.
  virtual std::shared_ptr<utils::graph::HostNode> getConnectDestination(int inputIndex);
  /// Updates host + core channelCount and renegotiates when the value changes.
  /// Safe to call from the host/JS thread only (negotiation reads these fields).
  void updateChannelCount(size_t newChannelCount);

  /// @brief The concrete audio-thread payload backing this host object.
  AudioNode *const audioNode_;

  const int numberOfInputs_;
  const int numberOfOutputs_;
  size_t channelCount_;
  ChannelCountMode channelCountMode_;
  ChannelInterpretation channelInterpretation_;
};
} // namespace audioapi
