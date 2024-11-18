#include "BaseAudioContext.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

AudioScheduledSourceNode::AudioScheduledSourceNode(BaseAudioContext *context)
    : AudioNode(context), isPlaying_(false) {
  numberOfInputs_ = 0;
}

void AudioScheduledSourceNode::start(double time) {
  nextChangeTime_ = time;
}

void AudioScheduledSourceNode::stop(double time) {
  nextChangeTime_ = time;
}

void AudioScheduledSourceNode::handlePlayback(double time, int framesToProcess) {
  if (nextChangeTime_ <= time + (framesToProcess / context_->getSampleRate())) {
    isPlaying_ = !isPlaying_;
  }
}

} // namespace audioapi
