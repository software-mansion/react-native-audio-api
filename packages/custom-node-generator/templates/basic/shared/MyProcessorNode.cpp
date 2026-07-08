#include "MyProcessorNode.h"

namespace audioapi {
MyProcessorNode::MyProcessorNode(
    const std::shared_ptr<BaseAudioContext> &context)
    : AudioNode(context) {}

void MyProcessorNode::processNode(int framesToProcess) {
  // put your processing logic here
}
} // namespace audioapi
