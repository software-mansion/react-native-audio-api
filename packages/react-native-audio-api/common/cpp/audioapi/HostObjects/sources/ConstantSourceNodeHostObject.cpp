#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>

namespace audioapi {

ConstantSourceNodeHostObject::ConstantSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConstantSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(
          context->getGraph(),
          std::make_unique<ConstantSourceNode>(context, options),
          options),
      constantSourceNode_(typedAudioNode<ConstantSourceNode>(node_)) {
  offsetParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, constantSourceNode_->getOffsetParam());

  addGetters(JSI_EXPORT_PROPERTY_GETTER(ConstantSourceNodeHostObject, offset));
}

JSI_PROPERTY_GETTER_IMPL(ConstantSourceNodeHostObject, offset) {
  return jsi::Object::createFromHostObject(runtime, offsetParam_);
}
} // namespace audioapi
