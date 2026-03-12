#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {

class MyProcessorNode : public AudioNode {
public:
  explicit MyProcessorNode(const std::shared_ptr<BaseAudioContext> &context, );

protected:
  std::shared_ptr<DSPAudioBuffer>
  processNode(const std::shared_ptr<DSPAudioBuffer> &buffer,
              int framesToProcess) override;
};
} // namespace audioapi
