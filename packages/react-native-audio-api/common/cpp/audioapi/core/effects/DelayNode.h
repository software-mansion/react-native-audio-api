#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>

#include <memory>

namespace audioapi {

class AudioBus;

class DelayNode : public AudioNode {
 public:
  explicit DelayNode(BaseAudioContext *context, float maxDelayTime);

  [[nodiscard]] std::shared_ptr<AudioParam> getDelayTimeParam() const;

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  void onInputDisabled() override;
  std::shared_ptr<AudioParam> delayTimeParam_;
  std::shared_ptr<AudioBus> delayBuffer_;
  size_t readIndex_ = 0;
  bool signalledToStop_ = false;
  int remainingFrames_ = 0;
};

} // namespace audioapi
