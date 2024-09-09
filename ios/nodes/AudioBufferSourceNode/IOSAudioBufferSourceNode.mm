#include <IOSAudioBufferSourceNode.h>

namespace audioapi {

IOSAudioBufferSourceNode::IOSAudioBufferSourceNode(AudioContext *context)
{
  audioNode_ = bufferSource_ = [[AudioBufferSourceNode alloc] initWithContext:context];
}

IOSAudioBufferSourceNode::~IOSAudioBufferSourceNode()
{
  [bufferSource_ cleanup];
  audioNode_ = bufferSource_ = nil;
}

void IOSAudioBufferSourceNode::start(double time) const
{
  [bufferSource_ start:time];
}

void IOSAudioBufferSourceNode::stop(double time) const
{
  [bufferSource_ stop:time];
}

void IOSAudioBufferSourceNode::setLoop(bool loop) const {
    bufferSource_.loop = loop;
}

bool IOSAudioBufferSourceNode::getLoop() {
    return bufferSource_.loop;
}

IOSAudioBuffer *IOSAudioBufferSourceNode::getBuffer() const {
    return nullptr;
}

void IOSAudioBufferSourceNode(IOSAudioBuffer *buffer) {
    return
}

} // namespace audioapi

