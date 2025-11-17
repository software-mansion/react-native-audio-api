#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <memory>

namespace audioapi {

ConstantSourceNodeHostObject::ConstantSourceNodeHostObject(
    const std::shared_ptr<ConstantSourceNode> &node)
    : AudioScheduledSourceNodeHostObject(node) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(ConstantSourceNodeHostObject, offset));
}

JSI_PROPERTY_GETTER_IMPL(ConstantSourceNodeHostObject, offset) {
  auto constantSourceNode = std::static_pointer_cast<ConstantSourceNode>(node_);
  auto offsetParam_ = std::make_shared<AudioParamHostObject>(constantSourceNode->getOffsetParam());
  return jsi::Object::createFromHostObject(runtime, offsetParam_);
}
} // namespace audioapi
