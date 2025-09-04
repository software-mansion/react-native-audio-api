#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/Constants.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(BaseAudioContext *context)
    : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  panParam_ = std::make_shared<AudioParam>(0.0, -1.0f, 1.0f, context);
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
  auto panParamValues = panParam_->processARateParam(framesToProcess, time)
                            ->getChannel(0)
                            ->getData();

  // Input is mono
  if (processingBus->getNumberOfChannels() == 1) {
    for (int i = 0; i < framesToProcess; i++) {
      auto pan = std::clamp(panParamValues[i], -1.0f, 1.0f);
      auto x = (pan + 1) / 2;

      auto gainL = static_cast<float>(cos(x * PI / 2));
      auto gainR = static_cast<float>(sin(x * PI / 2));

      float input = (*left)[i];

      (*left)[i] = input * gainL;
      (*right)[i] = input * gainR;
      time += deltaTime;
    }
  } else { // Input is stereo
    for (int i = 0; i < framesToProcess; i++) {
      auto pan = std::clamp(panParamValues[i], -1.0f, 1.0f);
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
}

} // namespace audioapi
