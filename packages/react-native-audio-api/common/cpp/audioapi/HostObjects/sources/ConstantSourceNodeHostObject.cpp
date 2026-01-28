#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/HostObjects/utils/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/ConstantSourceNode.h>
#include <memory>

namespace audioapi {

ConstantSourceNodeHostObject::ConstantSourceNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConstantSourceOptions &options)
    : AudioScheduledSourceNodeHostObject(context->createConstantSource(options)) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(ConstantSourceNodeHostObject, offset));
}

JSI_PROPERTY_GETTER_IMPL(ConstantSourceNodeHostObject, offset) {
  return jsi::Object::createFromHostObject(runtime, offsetParam_);
}
} // namespace audioapi
