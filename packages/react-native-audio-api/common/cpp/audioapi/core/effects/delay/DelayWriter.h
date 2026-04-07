#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/effects/delay/DelayLine.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

class BaseAudioContext;

/// Writes the node’s input into the delay ring at `writeIndex = (readIndex + delaySamples) % N`
/// (same rule as DelayNode).
class DelayWriter : public AudioNode {
 public:
  explicit DelayWriter(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioNodeOptions &options,
      std::shared_ptr<DelayLine> delayLine);

  void processNode(int framesToProcess) override;

 private:
  std::shared_ptr<DelayLine> delayLine_;
};

} // namespace audioapi
