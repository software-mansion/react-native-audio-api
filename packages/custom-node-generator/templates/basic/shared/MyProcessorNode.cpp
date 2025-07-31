#include "MyProcessorNode.h"
#include <audioapi/utils/AudioBus.h>

namespace audioapi {
MyProcessorNode::MyProcessorNode(BaseAudioContext *context)
    : AudioNode(context) {
    isInitialized_ = true;
}

void MyProcessorNode::processNode(const std::shared_ptr<AudioBus> &bus,
                                  int framesToProcess) {
    // put your processing logic here
}
} // namespace audioapi