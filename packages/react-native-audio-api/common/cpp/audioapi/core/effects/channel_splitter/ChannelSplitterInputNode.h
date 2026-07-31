#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {

class BaseAudioContext;

/// @brief The single input of a ChannelSplitterNode.
///
/// Sums every upstream connection into an N-channel bus using discrete
/// (index-based) channel mapping — the base `processInputs` already performs
/// the zero + discrete-sum because this node is configured with
/// channelCount = numberOfOutputs, explicit, discrete. Its output buffer is the
/// shared splitter bus. It is **not mixable** (`getOutput()` returns nullptr);
/// the per-channel output nodes read the bus directly, connecting to this node
/// only to enforce processing order.
class ChannelSplitterInputNode : public AudioNode {
 public:
  ChannelSplitterInputNode(const std::shared_ptr<BaseAudioContext> &context, int numberOfChannels);

  [[nodiscard]] const DSPAudioBuffer *getOutput() const override {
    return nullptr;
  }

 protected:
  void processNode(int framesToProcess) override;
};

} // namespace audioapi
