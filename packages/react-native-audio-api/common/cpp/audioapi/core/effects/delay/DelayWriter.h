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

 protected:
  /// @brief Tail length equals one full `maxDelayTime` worth of frames: the
  /// writer must keep filling the ring with zeros for at least that long
  /// after upstream goes silent so the reader can drain the audio currently
  /// stored in the ring.
  /// @note Audio Thread only.
  [[nodiscard]] int computeTailFrames() const override;

 private:
  std::shared_ptr<DelayLine> delayLine_;
};

} // namespace audioapi
