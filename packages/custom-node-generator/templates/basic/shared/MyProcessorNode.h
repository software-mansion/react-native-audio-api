#pragma once
#include <audioapi/core/AudioNode.h>

namespace audioapi {
class AudioBuffer;

class MyProcessorNode : public AudioNode {
public:
  explicit MyProcessorNode(BaseAudioContext *context);

protected:
  std::shared_ptr<AudioBuffer>
  processNode(const std::shared_ptr<AudioBuffer> &bus,
              int framesToProcess) override;
};
} // namespace audioapi
