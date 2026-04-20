# Reference: Complete GainNode C++ Example

This file contains the full GainNode header and implementation, extracted from `SKILL.md` to keep the main skill file under budget.

## `GainNode.h`

```cpp
#pragma once
#include "audioapi/core/AudioNode.h"
#include "audioapi/core/AudioParam.h"

namespace audioapi {

class GainNode : public AudioNode {
 public:
  // JS-thread only
  explicit GainNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const GainOptions &options);

  // JS-thread only
  [[nodiscard]] std::shared_ptr<AudioParam> getGainParam() const;

 protected:
  // Audio-thread only
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> gainParam_;
};

} // namespace audioapi
```

## `GainNode.cpp`

```cpp
GainNode::GainNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const GainOptions &options)
    : AudioNode(context, options) {
  // Preallocate param — constructor is on JS thread, allocation OK
  gainParam_ = std::make_shared<AudioParam>(
      options.gain,
      -3.4028234663852886e+38f,
      3.4028234663852886e+38f,
      context);
}

std::shared_ptr<AudioBuffer> GainNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (!context) return processingBuffer;

  double time = context->getCurrentTime();

  // A-rate: per-sample gain values — no allocation, reuses preallocated buffer
  auto gainParamValues = gainParam_->processARateParam(framesToProcess, time);
  auto gainValues = gainParamValues->getChannel(0);

  for (size_t i = 0; i < processingBuffer->getNumberOfChannels(); i++) {
    processingBuffer->getChannel(i)->multiply(*gainValues, framesToProcess);
  }

  return processingBuffer;
}
```

