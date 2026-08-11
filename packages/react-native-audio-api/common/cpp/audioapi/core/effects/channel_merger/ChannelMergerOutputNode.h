#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>
#include <vector>

namespace audioapi {

class BaseAudioContext;

/// @brief The single output of a ChannelMergerNode.
///
/// Its output buffer *is* the shared merger bus: the input nodes fill each
/// channel earlier in the topological order (they connect to this node purely
/// for ordering), so this node neither mixes nor zeroes anything — it only
/// exposes the already-assembled multi-channel bus to downstream nodes.
class ChannelMergerOutputNode : public AudioNode {
 public:
  ChannelMergerOutputNode(const std::shared_ptr<BaseAudioContext> &context, int numberOfChannels);

 protected:
  void processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames) override;
  void processNode(int framesToProcess) override;
};

} // namespace audioapi
