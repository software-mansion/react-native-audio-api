#include <audioapi/HostObjects/effects/GainNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/GainNode.h>
#include <memory>

namespace audioapi {

GainNodeHostObject::GainNodeHostObject(const std::shared_ptr<GainNode> &node)
    : AudioNodeHostObject(node), gainParam_(std::make_shared<AudioParamHostObject>(node->getGainParam())) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(GainNodeHostObject, gain));
}

JSI_PROPERTY_GETTER_IMPL(GainNodeHostObject, gain) {
  return jsi::Object::createFromHostObject(runtime, gainParam_);
}

} // namespace audioapi
