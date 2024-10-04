#include "StereoPannerNode.h"
#include "AudioContext.h"

namespace audioapi {

StereoPannerNode::StereoPannerNode(AudioContext *context) : AudioNode(context) {
  channelCountMode_ = "clamped-max";
  panParam_ = std::make_shared<AudioParam>(context, 0.0, -1.0, 1.0);
}

std::shared_ptr<AudioParam> StereoPannerNode::getPanParam() const {
  return panParam_;
}

void StereoPannerNode::process(
    AudioStream *oboeStream,
    void *audioData,
    int32_t numFrames,
    int channelCount) {
  auto *buffer = static_cast<float *>(audioData);

  for (int i = 0; i < numFrames; i++) {
    auto pan = panParam_->getValueAtTime(context_->getCurrentTime());
    auto x = (pan <= 0 ? pan + 1 : pan) * M_PI / 2;

    auto gainL = static_cast<float>(cos(x));
    auto gainR = static_cast<float>(sin(x));

    auto inputL = buffer[i * 2];
    auto inputR = buffer[i * 2 + 1];

    if (pan <= 0) {
      buffer[i * 2] += inputR * gainL;
      buffer[i * 2 + 1] *= gainR;
    } else {
      buffer[i * 2] *= gainL;
      buffer[i * 2 + 1] += inputL * gainR;
    }
  }

  AudioNode::process(oboeStream, audioData, numFrames, channelCount);
}

} // namespace audioapi
