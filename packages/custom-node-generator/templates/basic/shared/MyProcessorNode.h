#pragma once
#include <audioapi/core/AudioNode.h>

namespace audioapi {
class AudioBus;

class MyProcessorNode : public AudioNode {
public:
  explicit MyProcessorNode(BaseAudioContext *context);

protected:
  void processNode(const std::shared_ptr<AudioBus> &bus,
                   int framesToProcess) override;

};
} // namespace audioapi