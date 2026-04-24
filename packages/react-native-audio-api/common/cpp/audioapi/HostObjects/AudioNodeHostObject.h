#pragma once

#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/graph/HostNode.h>
#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/types/NodeOptions.h>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioNode;

class AudioNodeHostObject : public JsiHostObject, public utils::graph::HostNode {
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

  using utils::graph::HostNode::connect;
  using utils::graph::HostNode::disconnect;

  JSI_HOST_FUNCTION_DECL(connect);
  JSI_HOST_FUNCTION_DECL(disconnect);

  [[nodiscard]] virtual size_t getMemoryPressure() const {
    return 300'000; // magic number so node can be destroyed quite fast
  }

 protected:
  const int numberOfInputs_;
  const int numberOfOutputs_;
  size_t channelCount_;
  const ChannelCountMode channelCountMode_;
  const ChannelInterpretation channelInterpretation_;
};
} // namespace audioapi
