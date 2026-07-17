#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/utils/graph/HostNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

struct ChannelSplitterOptions;
class BaseAudioContext;
class ChannelSplitterInputNode;

/// @brief Composite host object for ChannelSplitterNode.
///
/// The splitter is realised as one input (bus) host node — this object's own
/// graph node — plus one mono host node per output. Incoming connections
/// terminate at the input node (the default); outgoing connections originate
/// from the matching output host node.
class ChannelSplitterNodeHostObject : public AudioNodeHostObject {
 public:
  explicit ChannelSplitterNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const ChannelSplitterOptions &options);

  [[nodiscard]] size_t getMemoryPressure() const override;

 protected:
  std::shared_ptr<utils::graph::HostNode> getConnectSource(int outputIndex) override;

 private:
  ChannelSplitterInputNode *inputNode_ = nullptr;
  std::vector<std::shared_ptr<utils::graph::HostNode>> outputHostNodes_;
};
} // namespace audioapi
