#include <IOSAudioBufferSourceNode.h>

namespace audioapi {

IOSAudioBufferSourceNode::IOSAudioBufferSourceNode(AudioBufferSourceNode *bufferSource)
{
  audioNode_ = audioScheduledSourceNode_ = bufferSource_ = bufferSource;
}

IOSAudioBufferSourceNode::~IOSAudioBufferSourceNode()
{
  [bufferSource_ cleanup];
  audioNode_ = audioScheduledSourceNode_ = bufferSource_ = nil;
}

void IOSAudioBufferSourceNode::setLoop(bool loop) const
{
  bufferSource_.loop = loop;
}

bool IOSAudioBufferSourceNode::getLoop()
{
  return bufferSource_.loop;
}

std::shared_ptr<IOSAudioBuffer> IOSAudioBufferSourceNode::getBuffer()
{
  auto buffer = bufferSource_.buffer;
  return std::make_shared<IOSAudioBuffer>(buffer);
}

void IOSAudioBufferSourceNode::setBuffer(std::shared_ptr<IOSAudioBuffer> buffer)
{
  bufferSource_.buffer = buffer->audioBuffer_;
}

} // namespace audioapi
