#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <memory>

namespace audioapi {

ConstantSourceNodeHostObject::ConstantSourceNodeHostObject(
    const std::shared_ptr<ConstantSourceNode> &node)
    : AudioScheduledSourceNodeHostObject(node),
    offsetParam_(std::make_shared<AudioParamHostObject>(node->getOffsetParam())) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(ConstantSourceNodeHostObject, offset));
}

JSI_PROPERTY_GETTER_IMPL(ConstantSourceNodeHostObject, offset) {
  return jsi::Object::createFromHostObject(runtime, offsetParam_);
}
} // namespace audioapi
