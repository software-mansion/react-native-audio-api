#include <audioapi/HostObjects/effects/StereoPannerNodeHostObject.h>

namespace audioapi {

JSI_PROPERTY_GETTER_IMPL(StereoPannerNodeHostObject, pan) {
  auto stereoPannerNode = std::static_pointer_cast<StereoPannerNode>(node_);
  auto panParam_ =
      std::make_shared<AudioParamHostObject>(stereoPannerNode->getPanParam());
  return jsi::Object::createFromHostObject(runtime, panParam_);
}

} // namespace audioapi
