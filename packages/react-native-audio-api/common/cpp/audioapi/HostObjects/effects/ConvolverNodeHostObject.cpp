#include <audioapi/HostObjects/effects/ConvolverNodeHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <memory>

namespace audioapi {

ConvolverNodeHostObject::ConvolverNodeHostObject(const std::shared_ptr<ConvolverNode> &node)
    : AudioNodeHostObject(node) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, normalize),
      JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, buffer));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(ConvolverNodeHostObject, normalize));
  addFunctions(JSI_EXPORT_FUNCTION(ConvolverNodeHostObject, setBuffer));
}

JSI_PROPERTY_GETTER_IMPL(ConvolverNodeHostObject, normalize) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  return {convolverNode->getNormalize_()};
}

JSI_PROPERTY_GETTER_IMPL(ConvolverNodeHostObject, buffer) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  auto buffer = convolverNode->getBuffer();
  auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);
  auto jsiObject = jsi::Object::createFromHostObject(runtime, bufferHostObject);
  jsiObject.setExternalMemoryPressure(runtime, bufferHostObject->getSizeInBytes() + 16);
  return jsiObject;
}

JSI_PROPERTY_SETTER_IMPL(ConvolverNodeHostObject, normalize) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  convolverNode->setNormalize(value.getBool());
}

JSI_HOST_FUNCTION_IMPL(ConvolverNodeHostObject, setBuffer) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  if (args[0].isUndefined()) {
    convolverNode->setBuffer(nullptr);
    return jsi::Value::undefined();
  }

  auto bufferHostObject = args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
  convolverNode->setBuffer(bufferHostObject->audioBuffer_);
  thisValue.asObject(runtime).setExternalMemoryPressure(
      runtime, bufferHostObject->getSizeInBytes() + 16);
  return jsi::Value::undefined();
}
} // namespace audioapi
