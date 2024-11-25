#include "BaseAudioContext.h"
#include "AudioNodeManager.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

AudioScheduledSourceNode::AudioScheduledSourceNode(BaseAudioContext *context)
    : AudioNode(context), playbackState_(PlaybackState::UNSCHEDULED) {
  numberOfInputs_ = 0;
}

void AudioScheduledSourceNode::start(double time) {
  context_->getNodeManager()->addSourceNode(shared_from_this());

  playbackState_ = PlaybackState::SCHEDULED;
  waitAndExecute(time, [this](double time) { startPlayback(); });
}

void AudioScheduledSourceNode::stop(double time) {
  waitAndExecute(time, [this](double time) { stopPlayback(); });
}

bool AudioScheduledSourceNode::isPlaying() {
  return playbackState_ == PlaybackState::PLAYING;
}

bool AudioScheduledSourceNode::isFinished() {
  return playbackState_ == PlaybackState::FINISHED;
}

void AudioScheduledSourceNode::startPlayback() {
  playbackState_ = PlaybackState::PLAYING;
}

void AudioScheduledSourceNode::stopPlayback() {
  playbackState_ = PlaybackState::FINISHED;
  disable();
}

void AudioScheduledSourceNode::waitAndExecute(
    double time,
    const std::function<void(double)> &fun) {
  std::thread([this, time, fun]() {
    while (context_->getCurrentTime() < time) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    fun(time);
  }).detach();
}

} // namespace audioapi
