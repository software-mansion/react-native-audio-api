#include <cassert>

#include "AudioArray.h"
#include "AudioBus.h"
#include "BaseAudioContext.h"
#include "Constants.h"
#include "StereoPannerNode.h"

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

namespace audioapi {

StereoPannerNode::StereoPannerNode(BaseAudioContext *context)
    : AudioNode(context) {
  channelCountMode_ = ChannelCountMode::CLAMPED_MAX;
  panParam_ = std::make_shared<AudioParam>(0.0, MIN_PAN, MAX_PAN);
  isInitialized_ = true;
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

void StereoPannerNode::processNode(
    AudioBus *processingBus,
    int framesToProcess) {
  // MIXING
  // TODO: Currently assumed channelCount is 2
  // it should:
  //  - support mono-channel buses
  //  - throw errors when trying to setup stereo panner with more than 2
  //  channels

  assert(processingBus->getNumberOfChannels() <= 2);

  double time = context_->getCurrentTime();
  double deltaTime = 1.0 / context_->getSampleRate();

  AudioArray *left = processingBus->getChannelByType(AudioBus::ChannelLeft);
  AudioArray *right = processingBus->getChannelByType(AudioBus::ChannelRight);

  for (int i = 0; i < framesToProcess; i += 1) {
    float pan = panParam_->getValueAtTime(time);
    float x = (pan <= 0 ? pan + 1 : pan);

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
