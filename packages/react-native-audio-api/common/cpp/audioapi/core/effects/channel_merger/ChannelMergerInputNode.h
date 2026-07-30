#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <memory>

namespace audioapi {

class BaseAudioContext;

/// @brief A single input of a ChannelMergerNode.
///
/// Each input downmixes everything connected to it to mono
/// (channelCount = 1, explicit, speakers, per the Web Audio spec) and writes
/// the result into one dedicated channel of the shared merger bus. The node is
/// intentionally **not mixable** — `getOutput()` returns nullptr — because the
/// merger's output node reads the assembled bus directly. The edge from each
/// input to the output node exists only to enforce processing order.
class ChannelMergerInputNode : public AudioNode {
 public:
  ChannelMergerInputNode(
      const std::shared_ptr<BaseAudioContext> &context,
      std::shared_ptr<DSPAudioBuffer> bus,
      size_t channelIndex);

  [[nodiscard]] const DSPAudioBuffer *getOutput() const override {
    return nullptr;
  }

 protected:
  void processNode(int framesToProcess) override;

 private:
  std::shared_ptr<DSPAudioBuffer> bus_;
  size_t channelIndex_;
};

} // namespace audioapi
