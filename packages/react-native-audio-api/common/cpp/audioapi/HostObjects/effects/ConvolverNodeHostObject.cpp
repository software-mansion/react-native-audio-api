#include <audioapi/HostObjects/effects/ConvolverNodeHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/effects/ConvolverNode.h>

namespace audioapi {

ConvolverNodeHostObject::ConvolverNodeHostObject(
    const std::shared_ptr<ConvolverNode> &node)
    : AudioNodeHostObject(node) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, normalize),
      JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, buffer));
  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(ConvolverNodeHostObject, normalize),
      JSI_EXPORT_PROPERTY_SETTER(ConvolverNodeHostObject, buffer));
}

JSI_PROPERTY_GETTER_IMPL(ConvolverNodeHostObject, normalize) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  return {convolverNode->getNormalize_()};
}

JSI_PROPERTY_GETTER_IMPL(ConvolverNodeHostObject, buffer) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  auto buffer = convolverNode->getBuffer();
  auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);
  return jsi::Object::createFromHostObject(runtime, bufferHostObject);
}

JSI_PROPERTY_SETTER_IMPL(ConvolverNodeHostObject, normalize) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  convolverNode->setNormalize(value.getBool());
}

JSI_PROPERTY_SETTER_IMPL(ConvolverNodeHostObject, buffer) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  if (value.isNull()) {
    convolverNode->setBuffer(nullptr);
    return;
  }

  auto bufferHostObject =
      value.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
  convolverNode->setBuffer(bufferHostObject->audioBuffer_);
}
} // namespace audioapi