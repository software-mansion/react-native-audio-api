#include <audioapi/HostObjects/effects/GainNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>

namespace audioapi {

GainNodeHostObject::GainNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const GainOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<GainNode>(context, options),
          options),
      gainNode_(typedAudioNode<GainNode>(node_)) {
  gainParam_ = std::make_shared<AudioParamHostObject>(graph_, node_, gainNode_->getGainParam());

  addGetters(JSI_EXPORT_PROPERTY_GETTER(GainNodeHostObject, gain));
}

JSI_PROPERTY_GETTER_IMPL(GainNodeHostObject, gain) {
  return jsi::Object::createFromHostObject(runtime, gainParam_);
}

} // namespace audioapi
