#include <IOSAudioDestinationNode.h>

namespace audiocontext {

IOSAudioDestinationNode::IOSAudioDestinationNode(std::shared_ptr<IOSAudioContext> context)
{
  audioNode_ = destination_ = [[AudioDestinationNode alloc] initWithContext:context->audioContext_];
}

IOSAudioDestinationNode::~IOSAudioDestinationNode()
{
  audioNode_ = destination_ = nil;
}
} // namespace audiocontext
