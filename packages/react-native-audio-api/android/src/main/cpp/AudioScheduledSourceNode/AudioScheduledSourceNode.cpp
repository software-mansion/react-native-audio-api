#include "AudioScheduledSourceNode.h"
#include "AudioContext.h"

namespace audioapi {

AudioScheduledSourceNode::AudioScheduledSourceNode(AudioContext *context)
    : AudioNode(context), isPlaying_(false) {
  numberOfInputs_ = 0;
}

void AudioScheduledSourceNode::start(double time) {
    waitAndExecute(time, [this](double time) {
        startPlayback();
    });
}

void AudioScheduledSourceNode::stop(double time) {
    waitAndExecute(time, [this](double time) {
        stopPlayback();
    });
}

void AudioScheduledSourceNode::cleanup() {
  AudioNode::cleanup();
}

void AudioScheduledSourceNode::startPlayback() {
    isPlaying_ = true;
}

void AudioScheduledSourceNode::stopPlayback() {
    isPlaying_ = false;
}

void AudioScheduledSourceNode::waitAndExecute(double time, const std::function<void(double)>& fun) {
    std::thread([this, time, fun]() {
        while(context_->getCurrentTime() < time) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        fun(time);
    }).detach();
}

} // namespace audioapi
