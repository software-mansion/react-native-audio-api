#include "AudioBus.h"
#include "Constants.h"
#include "AudioArray.h"
#include "StereoPannerNode.h"
#include "BaseAudioContext.h"

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(BaseAudioContext *context)
    : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  panParam_ = std::make_shared<AudioParam>(context, 0.0, -MAX_PAN, MAX_PAN);
  debugName_ = "StereoPannerNode";
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

void StereoPannerNode::processNode(AudioBus* processingBus, int framesToProcess) {
  // TODO: Currently assumed channelCount is 2
  // it should:
  //  - support mono-channel buses
  //  - throw errors when trying to setup stereo panner with more than 2 channels

  double time = context_->getCurrentTime();
  double deltaTime = 1.0 / context_->getSampleRate();

  AudioArray* left = processingBus->getChannelByType(AudioBus::ChannelLeft);
  AudioArray* right = processingBus->getChannelByType(AudioBus::ChannelRight);

  for (int i = 0; i < framesToProcess; i += 1) {
    float pan = panParam_->getValueAtTime(time);
    float x = (pan <= 0 ? pan + 1 : pan) * M_PI / 2;

    float gainL = static_cast<float>(cos(x));
    float gainR = static_cast<float>(sin(x));

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
