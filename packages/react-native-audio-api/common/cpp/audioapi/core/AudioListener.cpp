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
} // namespace audioapi
