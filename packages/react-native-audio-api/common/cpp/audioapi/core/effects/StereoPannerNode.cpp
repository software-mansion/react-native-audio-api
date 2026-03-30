#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <memory>

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const StereoPannerOptions &options)
    : AudioNode(context, options),
      panParam_(std::make_shared<AudioParam>(options.pan, -1.0f, 1.0f, context)),
      outputBuffer_(
          std::make_shared<DSPAudioBuffer>(
              RENDER_QUANTUM_SIZE,
              channelCount_,
              context->getSampleRate())) {}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

std::shared_ptr<DSPAudioBuffer> StereoPannerNode::getOutputBuffer() const {
  return outputBuffer_;
}

void StereoPannerNode::processNode(int framesToProcess) {
  std::shared_ptr<BaseAudioContext> context = context_.lock();
  if (context == nullptr) {
    return;
  }

  double time = context->getCurrentTime();
  double deltaTime = 1.0 / context->getSampleRate();

  auto panParamValues = panParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();

  auto outputLeft = outputBuffer_->getChannelByType(AudioBuffer::ChannelLeft)->span();
  auto outputRight = outputBuffer_->getChannelByType(AudioBuffer::ChannelRight)->span();

  // Input is mono
  if (audioBuffer_->getNumberOfChannels() == 1) {
    auto inputLeft = audioBuffer_->getChannelByType(AudioBuffer::ChannelMono)->span();

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
    auto inputLeft = audioBuffer_->getChannelByType(AudioBuffer::ChannelLeft)->span();
    auto inputRight = audioBuffer_->getChannelByType(AudioBuffer::ChannelRight)->span();

    for (int i = 0; i < framesToProcess; i++) {
      const auto pan = std::clamp(panParamValues[i], -1.0f, 1.0f);
      const auto x = (pan <= 0 ? pan + 1 : pan);
      const auto gainL = cos(x * PI / 2);
      const auto gainR = sin(x * PI / 2);
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
}

} // namespace audioapi
