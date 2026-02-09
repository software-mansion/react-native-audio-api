#include <audioapi/HostObjects/effects/ConvolverNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/ConvolverNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

ConvolverNodeHostObject::ConvolverNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const ConvolverOptions &options)
    : AudioNodeHostObject(context->createConvolver(options), options),
      normalize_(!options.disableNormalization) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(ConvolverNodeHostObject, normalize));
  addSetters(JSI_EXPORT_PROPERTY_SETTER(ConvolverNodeHostObject, normalize));
  addFunctions(JSI_EXPORT_FUNCTION(ConvolverNodeHostObject, setBuffer));
}

JSI_PROPERTY_GETTER_IMPL(ConvolverNodeHostObject, normalize) {
  return jsi::Value(normalize_);
}

JSI_PROPERTY_SETTER_IMPL(ConvolverNodeHostObject, normalize) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);
  auto normalize = value.getBool();

  auto event = [convolverNode, normalize](BaseAudioContext &) {
    convolverNode->setNormalize(normalize);
  };
  convolverNode->scheduleAudioEvent(std::move(event));
  normalize_ = normalize;
}

JSI_HOST_FUNCTION_IMPL(ConvolverNodeHostObject, setBuffer) {
  auto convolverNode = std::static_pointer_cast<ConvolverNode>(node_);

  std::shared_ptr<AudioBuffer> copiedBuffer;

  if (args[0].isObject()) {
    auto bufferHostObject = args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    thisValue.asObject(runtime).setExternalMemoryPressure(
        runtime, bufferHostObject->getSizeInBytes());
    copiedBuffer = std::make_shared<AudioBuffer>(*bufferHostObject->audioBuffer_);
  }
  auto event = [convolverNode, copiedBuffer](BaseAudioContext &) {
    convolverNode->setBuffer(copiedBuffer);
  };
  convolverNode->scheduleAudioEvent(std::move(event));

  return jsi::Value::undefined();
}
} // namespace audioapi
