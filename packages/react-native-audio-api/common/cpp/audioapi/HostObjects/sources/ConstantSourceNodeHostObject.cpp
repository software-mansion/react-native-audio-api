#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>

namespace audioapi {

ConstantSourceNodeHostObject::ConstantSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConstantSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(context->createConstantSource(options), options) {
  auto constantSourceNode = std::static_pointer_cast<ConstantSourceNode>(node_);
  offsetParam_ = std::make_shared<AudioParamHostObject>(constantSourceNode->getOffsetParam());

  addGetters(JSI_EXPORT_PROPERTY_GETTER(ConstantSourceNodeHostObject, offset));
}

JSI_PROPERTY_GETTER_IMPL(ConstantSourceNodeHostObject, offset) {
  return jsi::Object::createFromHostObject(runtime, offsetParam_);
}
} // namespace audioapi
