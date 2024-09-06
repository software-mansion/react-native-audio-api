#include <IOSAudioDestinationNode.h>

namespace audioapi {

IOSAudioDestinationNode::IOSAudioDestinationNode(AudioContext *context)
{
  audioNode_ = destination_ = [[AudioDestinationNode alloc] initWithContext:context];
}

IOSAudioDestinationNode::~IOSAudioDestinationNode()
{
  audioNode_ = destination_ = nil;
}
} // namespace audioapi
