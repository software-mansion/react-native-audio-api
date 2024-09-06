#include <IOSGainNode.h>

namespace audioapi {

IOSGainNode::IOSGainNode(AudioContext *context)
{
  audioNode_ = gainNode_ = [[GainNode alloc] initWithContext:context];
}

IOSGainNode::~IOSGainNode()
{
  [gainNode_ cleanup];
  audioNode_ = gainNode_ = nil;
}

std::shared_ptr<IOSAudioParam> IOSGainNode::getGainParam()
{
  std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>();
  param->audioParam = gainNode_.gainParam;
  return param;
}
} // namespace audioapi
