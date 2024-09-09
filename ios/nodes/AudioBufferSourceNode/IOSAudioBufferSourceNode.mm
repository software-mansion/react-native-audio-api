#include <IOSAudioBufferSourceNode.h>

namespace audioapi {

IOSAudioBufferSourceNode::IOSAudioBufferSourceNode(AudioBufferSourceNode *bufferSource)
{
  audioNode_ = bufferSource_ = bufferSource;
}

IOSAudioBufferSourceNode::~IOSAudioBufferSourceNode()
{
  [bufferSource_ cleanup];
  audioNode_ = bufferSource_ = nil;
}

void IOSAudioBufferSourceNode::setLoop(bool loop) const
{
  bufferSource_.loop = loop;
}

bool IOSAudioBufferSourceNode::getLoop()
{
  return bufferSource_.loop;
}

} // namespace audioapi
