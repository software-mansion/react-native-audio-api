#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <memory>

namespace audioapi {

class BaseAudioContext;

/// @brief One of the mono outputs of a ChannelSplitterNode.
///
/// Exposes a single channel of the shared splitter bus as a mono
/// (channelCount = 1, explicit) output. It connects *from* the splitter input
/// node purely for processing order; on each quantum it copies its channel of
/// the bus into its own mono output buffer. Channels beyond the bus width emit
/// silence (an inactive output).
class ChannelSplitterOutputNode : public AudioNode {
 public:
  ChannelSplitterOutputNode(
      const std::shared_ptr<BaseAudioContext> &context,
      std::shared_ptr<DSPAudioBuffer> bus,
      size_t channelIndex);

 protected:
  void processNode(int framesToProcess) override;

 private:
  std::shared_ptr<DSPAudioBuffer> bus_;
  size_t channelIndex_;
};

} // namespace audioapi
