#include "MyProcessorNode.h"
#include <audioapi/utils/AudioBuffer.h>

namespace audioapi {
MyProcessorNode::MyProcessorNode(BaseAudioContext *context)
    : AudioNode(context) {
  isInitialized_ = true;
}

std::shared_ptr<AudioBuffer>
MyProcessorNode::processNode(const std::shared_ptr<AudioBuffer> &bus,
                             int framesToProcess) {
  // put your processing logic here
}
} // namespace audioapi
