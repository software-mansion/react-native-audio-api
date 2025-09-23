#include <audioapi/HostObjects/effects/GainNodeHostObject.h>

namespace audioapi {

JSI_PROPERTY_GETTER_IMPL(GainNodeHostObject, gain) {
  auto gainNode = std::static_pointer_cast<GainNode>(node_);
  auto gainParam =
      std::make_shared<AudioParamHostObject>(gainNode->getGainParam());
  return jsi::Object::createFromHostObject(runtime, gainParam);
}

} // namespace audioapi