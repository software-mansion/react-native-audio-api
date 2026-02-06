#include <audioapi/HostObjects/effects/DelayNodeHostObject.h>
#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

DelayNodeHostObject::DelayNodeHostObject(const std::shared_ptr<BaseAudioContext>& context, const DelayOptions &options)
    : AudioNodeHostObject(context->createDelay(options), options) {
  auto delayNode = std::static_pointer_cast<DelayNode>(node_);
  delayTimeParam_ = std::make_shared<AudioParamHostObject>(delayNode->getDelayTimeParam());
  addGetters(JSI_EXPORT_PROPERTY_GETTER(DelayNodeHostObject, delayTime));
}

JSI_PROPERTY_GETTER_IMPL(DelayNodeHostObject, delayTime) {
    return jsi::Object::createFromHostObject(runtime, delayTimeParam_);
}

size_t DelayNodeHostObject::getSizeInBytes() const {
  auto delayNode = std::static_pointer_cast<DelayNode>(node_);
  auto base = sizeof(float) * delayNode->getDelayTimeParam()->getMaxValue();
  if (std::shared_ptr<BaseAudioContext> context = delayNode->context_.lock()) {
    return base * context->getSampleRate();
  } else {
    return base * 44100; // Fallback to common sample rate
  }
}

} // namespace audioapi
