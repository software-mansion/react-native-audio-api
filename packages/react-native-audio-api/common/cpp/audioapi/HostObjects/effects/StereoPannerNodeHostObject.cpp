#include <audioapi/HostObjects/effects/StereoPannerNodeHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/effects/StereoPannerNode.h>

namespace audioapi {

StereoPannerNodeHostObject::StereoPannerNodeHostObject(
    const std::shared_ptr<StereoPannerNode> &node)
    : AudioNodeHostObject(node) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(StereoPannerNodeHostObject, pan));
}

JSI_PROPERTY_GETTER_IMPL(StereoPannerNodeHostObject, pan) {
  auto stereoPannerNode = std::static_pointer_cast<StereoPannerNode>(node_);
  auto panParam_ =
      std::make_shared<AudioParamHostObject>(stereoPannerNode->getPanParam());
  return jsi::Object::createFromHostObject(runtime, panParam_);
}

} // namespace audioapi
