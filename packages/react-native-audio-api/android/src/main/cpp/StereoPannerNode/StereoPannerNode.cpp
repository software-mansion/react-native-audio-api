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

bool StereoPannerNode::processAudio(float *audioData, int32_t numFrames) {
  // assumed channelCount = 2
  if (!AudioNode::processAudio(audioData, numFrames)) {
    return false;
  }

  for (int i = 0; i < numFrames; i++) {
    auto pan = panParam_->getValueAtTime(context_->getCurrentTime());
    auto x = (pan <= 0 ? pan + 1 : pan) * M_PI / 2;

    auto gainL = static_cast<float>(cos(x));
    auto gainR = static_cast<float>(sin(x));

    auto inputL = audioData[i * 2];
    auto inputR = audioData[i * 2 + 1];

    if (pan <= 0) {
      audioData[i * 2] = inputL + inputR * gainL;
      audioData[i * 2 + 1] = inputR * gainR;
    } else {
      audioData[i * 2] = inputL * gainL;
      audioData[i * 2 + 1] = inputR + inputL * gainR;
    }
  }

  normalize(audioData, numFrames);

  return true;
}

void StereoPannerNode::normalize(float *audioData, int32_t numFrames) {
  auto maxValue = 1.0f;

  for (int i = 0; i < numFrames * channelCount_; i++) {
    maxValue = std::max(maxValue, std::abs(audioData[i]));
  }

  for (int i = 0; i < numFrames * channelCount_; i++) {
    audioData[i] /= maxValue;
  }
}
} // namespace audioapi
