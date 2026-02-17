#include <audioapi/types/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioBuffer.h>
#include <memory>

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const StereoPannerOptions &options)
    : AudioNode(context, options),
      panParam_(std::make_shared<AudioParam>(options.pan, -1.0f, 1.0f, context)) {
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

std::shared_ptr<AudioBuffer> StereoPannerNode::processNode(
    const std::shared_ptr<AudioBuffer> &processingBuffer,
    int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr)
    return processingBuffer;
  double time = context->getCurrentTime();
  double deltaTime = 1.0 / context->getSampleRate();

  auto panParamValues = panParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();

  auto outputLeft = audioBuffer_->getChannelByType(AudioBuffer::ChannelLeft)->span();
  auto outputRight = audioBuffer_->getChannelByType(AudioBuffer::ChannelRight)->span();

  // Input is mono
  if (processingBuffer->getNumberOfChannels() == 1) {
    auto inputLeft = processingBuffer->getChannelByType(AudioBuffer::ChannelMono)->span();

    for (int i = 0; i < framesToProcess; i++) {
      const auto pan = std::clamp(panParamValues[i], -1.0f, 1.0f);
      const auto x = (pan + 1) / 2;
      const auto angle = x * (PI / 2);
      const float input = inputLeft[i];

      outputLeft[i] = input * std::cos(angle);
      outputRight[i] = input * std::sin(angle);
      time += deltaTime;
    }
  } else { // Input is stereo
    auto inputLeft = processingBuffer->getChannelByType(AudioBuffer::ChannelLeft)->span();
    auto inputRight = processingBuffer->getChannelByType(AudioBuffer::ChannelRight)->span();

    for (int i = 0; i < framesToProcess; i++) {
      const auto pan = std::clamp(panParamValues[i], -1.0f, 1.0f);
      const auto x = (pan <= 0 ? pan + 1 : pan);
      const auto gainL = static_cast<float>(cos(x * PI / 2));
      const auto gainR = static_cast<float>(sin(x * PI / 2));
      const float inputL = inputLeft[i];
      const float inputR = inputRight[i];

      if (pan <= 0) {
        outputLeft[i] = inputL + inputR * gainL;
        outputRight[i] = inputR * gainR;
      } else {
        outputLeft[i] = inputL * gainL;
        outputRight[i] = inputR + inputL * gainR;
      }

      time += deltaTime;
    }
  }

  return audioBuffer_;
}

} // namespace audioapi
