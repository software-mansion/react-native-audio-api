#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {

struct DelayOptions;

class DelayNode : public AudioNode {
 public:
  explicit DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getDelayTimeParam() const;

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  void onInputDisabled() override;
  enum class BufferAction { READ, WRITE };
  void delayBufferOperation(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess,
      size_t &operationStartingIndex,
      BufferAction action);
  const std::shared_ptr<AudioParam> delayTimeParam_;
  std::shared_ptr<AudioBuffer> delayBuffer_;
  size_t readIndex_ = 0;
  bool signalledToStop_ = false;
  int remainingFrames_ = 0;
};

} // namespace audioapi
