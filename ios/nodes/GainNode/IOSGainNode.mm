#include <IOSGainNode.h>

namespace audioapi {

IOSGainNode::IOSGainNode(GainNode *gainNode)
{
  audioNode_ = gainNode_ = gainNode;
}

IOSGainNode::~IOSGainNode()
{
  [gainNode_ cleanup];
  audioNode_ = gainNode_ = nil;
}

std::shared_ptr<IOSAudioParam> IOSGainNode::getGainParam()
{
  std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>(gainNode_.gainParam);
  return param;
}
} // namespace audioapi
