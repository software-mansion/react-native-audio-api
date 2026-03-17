#pragma once
#include <audioapi/core/AudioNode.h>

namespace audioapi {
class AudioBuffer;

class MyProcessorNode : public AudioNode {
public:
  explicit MyProcessorNode(const std::shared_ptr<BaseAudioContext> &context, );

protected:
  std::shared_ptr<AudioBuffer>
  processNode(const std::shared_ptr<AudioBuffer> &buffer,
              int framesToProcess) override;
};
} // namespace audioapi
