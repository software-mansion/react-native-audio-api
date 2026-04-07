#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/effects/delay/DelayReader.h>
#include <audioapi/core/effects/delay/DelayWriter.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {

struct DelayOptions;
class DelayLine;

class DelayNode : public AudioNode {
 public:
  explicit DelayNode(const std::shared_ptr<BaseAudioContext> &context, const DelayOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getDelayTimeParam() const;

  std::shared_ptr<DelayLine> delayLine_;
  std::unique_ptr<DelayReader> delayReader_;
  std::unique_ptr<DelayWriter> delayWriter_;

 protected:
  void processNode(int framesToProcess) override {
    // noop
  };

 private:
  enum class BufferAction : uint8_t { READ, WRITE };
  const std::shared_ptr<AudioParam> delayTimeParam_;
  std::shared_ptr<AudioBuffer> delayBuffer_;
  size_t readIndex_ = 0;
  bool signalledToStop_ = false;
  int remainingFrames_ = 0;
};

} // namespace audioapi
