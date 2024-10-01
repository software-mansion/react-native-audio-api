#include "AudioScheduledSourceNode.h"

namespace audioapi {

AudioScheduledSourceNode::AudioScheduledSourceNode() : AudioNode() {
    numberOfInputs_ = 0;
}

void AudioScheduledSourceNode::start(double time) {
  mStream->requestStart();
}

void AudioScheduledSourceNode::stop(double time) {
  mStream->requestStop();
}

} // namespace audioapi
