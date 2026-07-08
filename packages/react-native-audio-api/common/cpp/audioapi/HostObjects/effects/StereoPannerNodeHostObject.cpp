#include <audioapi/HostObjects/effects/StereoPannerNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/StereoPannerNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>

namespace audioapi {

StereoPannerNodeHostObject::StereoPannerNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const StereoPannerOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<StereoPannerNode>(context, options),
          options),
      stereoPannerNode_(typedAudioNode<StereoPannerNode>(node_)) {
  panParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, stereoPannerNode_->getPanParam());

  addGetters(JSI_EXPORT_PROPERTY_GETTER(StereoPannerNodeHostObject, pan));
}

JSI_PROPERTY_GETTER_IMPL(StereoPannerNodeHostObject, pan) {
  return jsi::Object::createFromHostObject(runtime, panParam_);
}

} // namespace audioapi
