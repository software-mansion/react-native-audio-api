#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>

namespace audioapi {

AudioBufferSourceNodeHostObject::AudioBufferSourceNodeHostObject(
    const std::shared_ptr<AudioBufferSourceNode> &node)
    : AudioBufferBaseSourceNodeHostObject(node) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopSkip),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, buffer),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopStart),
      JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopEnd));

  addSetters(
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loop),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopSkip),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopStart),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopEnd),
      JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, onLoopEnded));

  // start method is overridden in this class
  functions_->erase("start");

  addFunctions(
      JSI_EXPORT_FUNCTION(AudioBufferSourceNodeHostObject, start),
      JSI_EXPORT_FUNCTION(AudioBufferSourceNodeHostObject, setBuffer));
}

AudioBufferSourceNodeHostObject::~AudioBufferSourceNodeHostObject() {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);

  // When JSI object is garbage collected (together with the eventual callback),
  // underlying source node might still be active and try to call the
  // non-existing callback.
  audioBufferSourceNode->setOnLoopEndedCallbackId(0);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loop) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loop = audioBufferSourceNode->getLoop();
  return {loop};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loopSkip) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loopSkip = audioBufferSourceNode->getLoopSkip();
  return {loopSkip};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, buffer) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto buffer = audioBufferSourceNode->getBuffer();

  if (!buffer) {
    return jsi::Value::null();
  }

  auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);
  return jsi::Object::createFromHostObject(runtime, bufferHostObject);
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loopStart) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loopStart = audioBufferSourceNode->getLoopStart();
  return {loopStart};
}

JSI_PROPERTY_GETTER_IMPL(AudioBufferSourceNodeHostObject, loopEnd) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  auto loopEnd = audioBufferSourceNode->getLoopEnd();
  return {loopEnd};
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loop) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  audioBufferSourceNode->setLoop(value.getBool());
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopSkip) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  audioBufferSourceNode->setLoopSkip(value.getBool());
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopStart) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  audioBufferSourceNode->setLoopStart(value.getNumber());
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, loopEnd) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);
  audioBufferSourceNode->setLoopEnd(value.getNumber());
}

JSI_PROPERTY_SETTER_IMPL(AudioBufferSourceNodeHostObject, onLoopEnded) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);

  audioBufferSourceNode->setOnLoopEndedCallbackId(
      std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, start) {
  auto when = args[0].getNumber();
  auto offset = args[1].getNumber();

  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);

  if (args[2].isUndefined()) {
    audioBufferSourceNode->start(when, offset);

    return jsi::Value::undefined();
  }

  auto duration = args[2].getNumber();
  audioBufferSourceNode->start(when, offset, duration);

  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(AudioBufferSourceNodeHostObject, setBuffer) {
  auto audioBufferSourceNode =
      std::static_pointer_cast<AudioBufferSourceNode>(node_);

  if (args[0].isNull()) {
    audioBufferSourceNode->setBuffer(std::shared_ptr<AudioBuffer>(nullptr));
    return jsi::Value::undefined();
  }

  auto bufferHostObject =
      args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
  thisValue.asObject(runtime).setExternalMemoryPressure(
      runtime, bufferHostObject->getSizeInBytes() + 16);
  audioBufferSourceNode->setBuffer(bufferHostObject->audioBuffer_);
  return jsi::Value::undefined();
}

} // namespace audioapi
