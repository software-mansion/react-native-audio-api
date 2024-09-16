#include <IOSAudioDestinationNode.h>

namespace audioapi {

IOSAudioDestinationNode::IOSAudioDestinationNode(AudioDestinationNode *destination)
{
  audioNode_ = destination_ = destination;
}

IOSAudioDestinationNode::~IOSAudioDestinationNode()
{
  audioNode_ = destination_ = nil;
}
} // namespace audioapi
