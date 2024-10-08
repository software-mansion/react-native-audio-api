#include "StereoPannerNode.h"
#include "AudioContext.h"

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(AudioContext *context) : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  panParam_ = std::make_shared<AudioParam>(context, 0.0, -MAX_PAN, MAX_PAN);
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

void StereoPannerNode::processAudio() {
    //channelCount = 2
    AudioNode::processAudio();

    for (int i = 0; i < context_->getBufferSize(); i++) {
        auto pan = panParam_->getValueAtTime(context_->getCurrentTime());
        auto x = (pan <= 0 ? pan + 1 : pan) * M_PI / 2;

        auto gainL = static_cast<float>(cos(x));
        auto gainR = static_cast<float>(sin(x));

        auto inputL = inputBuffer_[i * 2];
        auto inputR = inputBuffer_[i * 2 + 1];

    if (pan <= 0) {
      outputBuffer_[i * 2] = inputL + inputR * gainL;
      outputBuffer_[i * 2 + 1] = inputR * gainR;
    } else {
        outputBuffer_[i * 2] = inputL * gainL;
        outputBuffer_[i * 2 + 1] = inputR + inputL * gainR;
    }
  }
}

} // namespace audioapi
