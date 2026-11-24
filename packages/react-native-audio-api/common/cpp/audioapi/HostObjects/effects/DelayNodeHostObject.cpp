#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
#include <audioapi/HostObjects/effects/DelayNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

DelayNodeHostObject::DelayNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const DelayOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<DelayNode>(context, options),
          options),
      delayNode_(typedAudioNode<DelayNode>(node_)) {
  delayTimeParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, delayNode_->getDelayTimeParam());

  addGetters(JSI_EXPORT_PROPERTY_GETTER(DelayNodeHostObject, delayTime));
}

JSI_PROPERTY_GETTER_IMPL(DelayNodeHostObject, delayTime) {
  return jsi::Object::createFromHostObject(runtime, delayTimeParam_);
}

size_t DelayNodeHostObject::getMemoryPressure() const {
  const size_t delayBufferBytes = delayNode_->getDelayBufferSize() * channelCount_ * sizeof(float);
  return AudioNodeHostObject::getMemoryPressure() + kAudioParamBytes + delayBufferBytes;
}

} // namespace audioapi
