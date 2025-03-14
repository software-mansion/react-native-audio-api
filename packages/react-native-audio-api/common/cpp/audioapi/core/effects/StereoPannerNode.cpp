#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(BaseAudioContext *context)
    : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::EXPLICIT;
  panParam_ = std::make_shared<AudioParam>(0.0, -1.0f, 1.0f);
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

void StereoPannerNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  double time = context_->getCurrentTime();
  double deltaTime = 1.0 / context_->getSampleRate();

  AudioArray *left = processingBus->getChannelByType(AudioBus::ChannelLeft);
  AudioArray *right = processingBus->getChannelByType(AudioBus::ChannelRight);

  for (int i = 0; i < framesToProcess; i += 1) {
    auto pan = panParam_->getValueAtTime(time);

    auto x = (pan <= 0 ? pan + 1 : pan);

    auto gainL = static_cast<float>(cos(x * PI / 2));
    auto gainR = static_cast<float>(sin(x * PI / 2));

    float inputL = (*left)[i];
    float inputR = (*right)[i];

    if (pan <= 0) {
      (*left)[i] = inputL + inputR * gainL;
      (*right)[i] = inputR * gainR;
    } else {
      (*left)[i] = inputL * gainL;
      (*right)[i] = inputR + inputL * gainR;
    }

    time += deltaTime;
  }
}

} // namespace audioapi
