#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>

#include <functional>
#include <memory>

namespace audioapi {

class AudioBus;

class DelayNode : public AudioNode {
 public:
  explicit DelayNode(std::shared_ptr<BaseAudioContext> context, float maxDelayTime);

  [[nodiscard]] std::shared_ptr<AudioParam> getDelayTimeParam() const;

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  void onInputDisabled() override;
  enum class BufferAction { READ, WRITE };
  void delayBufferOperation(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess,
      size_t &operationStartingIndex,
      BufferAction action);
  std::shared_ptr<AudioParam> delayTimeParam_;
  std::shared_ptr<AudioBus> delayBuffer_;
  size_t readIndex_ = 0;
  bool signalledToStop_ = false;
  int remainingFrames_ = 0;
};

} // namespace audioapi
