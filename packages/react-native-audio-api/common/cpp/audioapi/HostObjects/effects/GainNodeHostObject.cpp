#include <audioapi/HostObjects/effects/GainNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/utils/NodeOptions.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/GainNode.h>
#include <memory>

namespace audioapi {

GainNodeHostObject::GainNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const GainOptions &options)
    : AudioNodeHostObject(context->createGain(options)) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(GainNodeHostObject, gain));
}

JSI_PROPERTY_GETTER_IMPL(GainNodeHostObject, gain) {
  return jsi::Object::createFromHostObject(runtime, gainParam_);
}

} // namespace audioapi
