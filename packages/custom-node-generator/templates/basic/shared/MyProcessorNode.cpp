#include "MyProcessorNode.h"
#include <audioapi/utils/AudioBus.h>

namespace audioapi {
MyProcessorNode::MyProcessorNode(std::shared_ptr<BaseAudioContext> context)
    : AudioNode(context) {
    isInitialized_ = true;
}

std::shared_ptr<AudioBus> MyProcessorNode::processNode(const std::shared_ptr<AudioBus> &bus,
                                  int framesToProcess) {
    // put your processing logic here
}
} // namespace audioapi
