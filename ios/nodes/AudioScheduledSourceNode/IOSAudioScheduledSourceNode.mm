#include <IOSAudioScheduledSourceNode.h>

namespace audioapi {
IOSAudioScheduledSourceNode::~IOSAudioScheduledSourceNode()
{
  [audioScheduledSourceNode_ cleanup];
  audioNode_ = audioScheduledSourceNode_ = nil;
}

void IOSAudioScheduledSourceNode::start(double time) const {
    [audioScheduledSourceNode_ start:time];
}

void IOSAudioScheduledSourceNode::stop(double time) const {
    [audioScheduledSourceNode_ stop:time];
}
} // namespace audioapi
