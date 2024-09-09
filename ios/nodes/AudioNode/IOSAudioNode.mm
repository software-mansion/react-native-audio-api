#include <IOSAudioNode.h>

namespace audioapi {
IOSAudioNode::~IOSAudioNode()
{
  [audioNode_ cleanup];
  audioNode_ = nil;
}

void IOSAudioNode::connect(std::shared_ptr<IOSAudioNode> node)
{
  [audioNode_ connect:(node->audioNode_)];
}

void IOSAudioNode::disconnect(std::shared_ptr<IOSAudioNode> node)
{
  [audioNode_ disconnect:(node->audioNode_)];
}

int IOSAudioNode::getNumberOfInputs()
{
  return [audioNode_ getNumberOfInputs];
}

int IOSAudioNode::getNumberOfOutputs()
{
  return [audioNode_ getNumberOfOutputs];
}

int IOSAudioNode::getChannelCount() const{
    return [audioNode_ getChannelCount];
}

std::string IOSAudioNode::getChannelCountMode() const {
    NSString *nsType = [audioNode_ getChannelCountMode];
    return std::string([nsType UTF8String]);
}

std::string IOSAudioNode::getChannelInterpretation() const {
    NSString *nsType = [audioNode_ getChannelInterpretation];
    return std::string([nsType UTF8String]);
}
} // namespace audioapi
