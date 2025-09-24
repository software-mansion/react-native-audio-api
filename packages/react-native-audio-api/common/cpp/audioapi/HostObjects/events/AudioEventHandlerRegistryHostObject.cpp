#include <audioapi/HostObjects/events/AudioEventHandlerRegistryHostObject.h>

#include <audioapi/events/AudioEventHandlerRegistry.h>

namespace audioapi {

AudioEventHandlerRegistryHostObject::AudioEventHandlerRegistryHostObject(
    const std::shared_ptr<AudioEventHandlerRegistry> &eventHandlerRegistry) {
  eventHandlerRegistry_ = eventHandlerRegistry;

  addFunctions(
      JSI_EXPORT_FUNCTION(
          AudioEventHandlerRegistryHostObject, addAudioEventListener),
      JSI_EXPORT_FUNCTION(
          AudioEventHandlerRegistryHostObject, removeAudioEventListener));
}

JSI_HOST_FUNCTION_IMPL(
    AudioEventHandlerRegistryHostObject,
    addAudioEventListener) {
  auto eventName = args[0].getString(runtime).utf8(runtime);
  auto callback = std::make_shared<jsi::Function>(
      args[1].getObject(runtime).getFunction(runtime));

  auto listenerId = eventHandlerRegistry_->registerHandler(eventName, callback);

  return jsi::String::createFromUtf8(runtime, std::to_string(listenerId));
}

JSI_HOST_FUNCTION_IMPL(
    AudioEventHandlerRegistryHostObject,
    removeAudioEventListener) {
  auto eventName = args[0].getString(runtime).utf8(runtime);
  uint64_t listenerId = std::stoull(args[1].getString(runtime).utf8(runtime));

  eventHandlerRegistry_->unregisterHandler(eventName, listenerId);

  return jsi::Value::undefined();
}

} // namespace audioapi
