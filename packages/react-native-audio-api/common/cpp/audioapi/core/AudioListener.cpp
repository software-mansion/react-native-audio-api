#include <audioapi/core/AudioListener.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/Constants.h>

#include <memory>

// https://webaudio.github.io/web-audio-api/#AudioListener

namespace audioapi {

namespace {

std::shared_ptr<AudioParam> makeListenerParam(
    float defaultValue,
    const std::shared_ptr<BaseAudioContext> &context) {
  return std::make_shared<AudioParam>(
      defaultValue, MOST_NEGATIVE_SINGLE_FLOAT, MOST_POSITIVE_SINGLE_FLOAT, context);
}

} // namespace

AudioListener::AudioListener(const std::shared_ptr<BaseAudioContext> &context)
    : positionXParam_(makeListenerParam(0.0f, context)),
      positionYParam_(makeListenerParam(0.0f, context)),
      positionZParam_(makeListenerParam(0.0f, context)),
      forwardXParam_(makeListenerParam(0.0f, context)),
      forwardYParam_(makeListenerParam(0.0f, context)),
      forwardZParam_(makeListenerParam(-1.0f, context)),
      upXParam_(makeListenerParam(0.0f, context)),
      upYParam_(makeListenerParam(1.0f, context)),
      upZParam_(makeListenerParam(0.0f, context)) {}

void AudioListener::processForQuantum(int framesToProcess, double time, std::size_t sampleFrame) {
  if (lastProcessedSampleFrame_.has_value() && *lastProcessedSampleFrame_ == sampleFrame) {
    return;
  }

  lastProcessedSampleFrame_ = sampleFrame;
  positionXValues_ =
      positionXParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  positionYValues_ =
      positionYParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  positionZValues_ =
      positionZParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  forwardXValues_ = forwardXParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  forwardYValues_ = forwardYParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  forwardZValues_ = forwardZParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  upXValues_ = upXParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  upYValues_ = upYParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
  upZValues_ = upZParam_->processARateParam(framesToProcess, time)->getChannel(0)->span();
}

} // namespace audioapi
