#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/utils/graph/HostNode.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

struct ChannelMergerOptions;
class BaseAudioContext;
class ChannelMergerOutputNode;

/// @brief Composite host object for ChannelMergerNode.
///
/// Mirrors the DelayNode composite pattern: the merger is realised as one
/// output (bus) host node — this object's own graph node — plus one host node
/// per input. Incoming connections are routed to the matching input host node;
/// outgoing connections originate from the output node (the default).
class ChannelMergerNodeHostObject : public AudioNodeHostObject {
 public:
  explicit ChannelMergerNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const ChannelMergerOptions &options);

  [[nodiscard]] size_t getMemoryPressure() const override;

 protected:
  std::shared_ptr<utils::graph::HostNode> getInput(int inputIndex) override;

 private:
  ChannelMergerOutputNode *outputNode_ = nullptr;
  std::vector<std::shared_ptr<utils::graph::HostNode>> inputHostNodes_;
};
} // namespace audioapi
