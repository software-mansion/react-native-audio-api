#include "AudioArray.h"
#include "AudioBus.h"
#include "AudioUtils.h"
#include "AudioScheduledSourceNode.h"
#include "AudioNodeManager.h"
#include "BaseAudioContext.h"

namespace audioapi {

AudioScheduledSourceNode::AudioScheduledSourceNode(BaseAudioContext *context)
    : AudioNode(context), playbackState_(PlaybackState::UNSCHEDULED), startFrame_(-1), stopFrame_(-1) {
  numberOfInputs_ = 0;
}

void AudioScheduledSourceNode::start(double time) {
  playbackState_ = PlaybackState::SCHEDULED;
  startFrame_ = AudioUtils::timeToSampleFrame(time, context_->getSampleRate());
  context_->getNodeManager()->addSourceNode(shared_from_this());
}

void AudioScheduledSourceNode::stop(double time) {
  stopFrame_ = AudioUtils::timeToSampleFrame(time, context_->getSampleRate());
}

bool AudioScheduledSourceNode::isUnscheduled() {
  return playbackState_ == PlaybackState::UNSCHEDULED;
}

bool AudioScheduledSourceNode::isScheduled() {
  return playbackState_ == PlaybackState::SCHEDULED;
}


bool AudioScheduledSourceNode::isPlaying() {
  return playbackState_ == PlaybackState::PLAYING;
}

bool AudioScheduledSourceNode::isFinished() {
  return playbackState_ == PlaybackState::FINISHED;
}

void AudioScheduledSourceNode::updatePlaybackInfo(AudioBus *processingBus, int framesToProcess, size_t& startOffset, size_t& nonSilentFramesToProcess) {
  size_t firstFrame = context_->getCurrentSampleFrame();
  size_t lastFrame = firstFrame + framesToProcess;
  size_t startFrame = std::max(startFrame_, firstFrame);
  size_t stopFrame = stopFrame_;

  if (isUnscheduled() || isFinished()) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;
    return;
  }

  if (isScheduled()) {
    // not yet playing
    if (startFrame > lastFrame) {
      startOffset = 0;
      nonSilentFramesToProcess = 0;
      return;
    }

    // start playing
    // zero first frames before starting frame
    playbackState_ = PlaybackState::PLAYING;
    startOffset = std::max(0ul, std::max(startFrame, firstFrame) - firstFrame);
    nonSilentFramesToProcess = std::min(lastFrame, stopFrame) - startFrame;
    processingBus->zero(0, startOffset);
    return;
  }

  // isPlaying() == true

  // stop will happen in this render quantum
  // zero remaining frames after stop frame
  if (stopFrame < lastFrame && stopFrame >= firstFrame) {
    startOffset = 0;
    nonSilentFramesToProcess = stopFrame - firstFrame;
    processingBus->zero(stopFrame - firstFrame, lastFrame - stopFrame);
    return;
  }

  // mark as finished in first silent render quantum
  if (stopFrame < firstFrame) {
    startOffset = 0;
    nonSilentFramesToProcess = 0;
    playbackState_ = PlaybackState::FINISHED;
    return;
  }

  // normal "mid-buffer" playback
  startOffset = 0;
  nonSilentFramesToProcess = framesToProcess;
}

} // namespace audioapi
