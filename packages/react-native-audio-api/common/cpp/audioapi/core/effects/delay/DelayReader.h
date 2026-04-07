#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/effects/delay/DelayLine.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

class BaseAudioContext;

/// Reads from the delay line at `delayLine->readIndex()` into the node buffer
class DelayReader : public AudioNode {
 public:
  explicit DelayReader(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioNodeOptions &options,
      std::shared_ptr<DelayLine> delayLine);

  void processNode(int framesToProcess) override;

 private:
  std::shared_ptr<DelayLine> delayLine_;
};

} // namespace audioapi
