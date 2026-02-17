#include "MyProcessorNode.h"
#include <audioapi/utils/AudioBuffer.h>

namespace audioapi {
MyProcessorNode::MyProcessorNode(
    const std::shared_ptr<BaseAudioContext> &context, )
    : AudioNode(context) {
  isInitialized_ = true;
}

std::shared_ptr<AudioBuffer>
MyProcessorNode::processNode(const std::shared_ptr<AudioBuffer> &buffer,
                             int framesToProcess) {
  // put your processing logic here
}
} // namespace audioapi
