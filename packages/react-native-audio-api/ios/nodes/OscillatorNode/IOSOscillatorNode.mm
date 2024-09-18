#include <IOSOscillatorNode.h>

namespace audioapi {

IOSOscillatorNode::IOSOscillatorNode(OscillatorNode *oscillator)
{
  audioNode_ = audioScheduledSourceNode_ = oscillatorNode_ = oscillator;
}

IOSOscillatorNode::~IOSOscillatorNode()
{
  [oscillatorNode_ cleanup];
  audioNode_ = audioScheduledSourceNode_ = oscillatorNode_ = nil;
}

void IOSOscillatorNode::setType(const std::string &type) const
{
  NSString *nsType = [NSString stringWithUTF8String:type.c_str()];
  [oscillatorNode_ setType:nsType];
}

std::string IOSOscillatorNode::getType() const
{
  NSString *nsType = [oscillatorNode_ getType];
  return std::string([nsType UTF8String]);
}

std::shared_ptr<IOSAudioParam> IOSOscillatorNode::getFrequencyParam()
{
  std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>(oscillatorNode_.frequencyParam);
  return param;
}

std::shared_ptr<IOSAudioParam> IOSOscillatorNode::getDetuneParam()
{
  std::shared_ptr<IOSAudioParam> param = std::make_shared<IOSAudioParam>(oscillatorNode_.detuneParam);
  return param;
}
} // namespace audioapi
