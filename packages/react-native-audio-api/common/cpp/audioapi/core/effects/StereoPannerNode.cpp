#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBus.h>
#include <memory>

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(std::shared_ptr<BaseAudioContext> context)
    : AudioNode(context), panParam_(std::make_shared<AudioParam>(0.0, -1.0f, 1.0f, context)) {
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

std::shared_ptr<AudioBus> StereoPannerNode::processNode(
    const std::shared_ptr<AudioBus> &processingBus,
    int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return processingBus;
  double time = context->getCurrentTime();
  double deltaTime = 1.0 / context->getSampleRate();

  auto *inputLeft = processingBus->getChannelByType(AudioBus::ChannelLeft);
  auto panParamValues =
      panParam_->processARateParam(framesToProcess, time)->getChannel(0)->getData();

  auto *outputLeft = audioBus_->getChannelByType(AudioBus::ChannelLeft);
  auto *outputRight = audioBus_->getChannelByType(AudioBus::ChannelRight);

  // Input is mono
  if (processingBus->getNumberOfChannels() == 1) {
    for (int i = 0; i < framesToProcess; i++) {
      auto pan = std::clamp(panParamValues[i], -1.0f, 1.0f);
      auto x = (pan + 1) / 2;

      auto gainL = static_cast<float>(cos(x * PI / 2));
      auto gainR = static_cast<float>(sin(x * PI / 2));

      float input = (*inputLeft)[i];

      (*outputLeft)[i] = input * gainL;
      (*outputRight)[i] = input * gainR;
      time += deltaTime;
    }
  } else { // Input is stereo
    auto *inputRight = processingBus->getChannelByType(AudioBus::ChannelRight);
    for (int i = 0; i < framesToProcess; i++) {
      auto pan = std::clamp(panParamValues[i], -1.0f, 1.0f);
      auto x = (pan <= 0 ? pan + 1 : pan);

      auto gainL = static_cast<float>(cos(x * PI / 2));
      auto gainR = static_cast<float>(sin(x * PI / 2));

      float inputL = (*inputLeft)[i];
      float inputR = (*inputRight)[i];

      if (pan <= 0) {
        (*outputLeft)[i] = inputL + inputR * gainL;
        (*outputRight)[i] = inputR * gainR;
      } else {
        (*outputLeft)[i] = inputL * gainL;
        (*outputRight)[i] = inputR + inputL * gainR;
      }

      time += deltaTime;
    }
  }

  return audioBus_;
}

} // namespace audioapi
