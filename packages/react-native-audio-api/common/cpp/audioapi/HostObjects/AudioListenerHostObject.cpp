#include <audioapi/HostObjects/AudioListenerHostObject.h>

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/core/AudioListener.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/graph/GraphObject.h>

#include <memory>

namespace audioapi {

namespace {

/// Internal graph anchor for listener AudioParam bridge nodes. Not an AudioNode.
class ListenerGraphAnchor final : public utils::graph::GraphObject {};
// important: https://webaudio.github.io/web-audio-api/#listenerprocessing
// take this into an account when implementing panner node

} // namespace

AudioListenerHostObject::AudioListenerHostObject(const std::shared_ptr<BaseAudioContext> &context)
    : utils::graph::HostNode(context->getGraph(), std::make_unique<ListenerGraphAnchor>()),
      listener_(std::make_unique<AudioListener>(context)) {
  positionXParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getPositionXParam());
  positionYParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getPositionYParam());
  positionZParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getPositionZParam());
  forwardXParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getForwardXParam());
  forwardYParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getForwardYParam());
  forwardZParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getForwardZParam());
  upXParam_ = std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getUpXParam());
  upYParam_ = std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getUpYParam());
  upZParam_ = std::make_shared<AudioParamHostObject>(graph_, node_, listener_->getUpZParam());

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, positionX),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, positionY),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, positionZ),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, forwardX),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, forwardY),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, forwardZ),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, upX),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, upY),
      JSI_EXPORT_PROPERTY_GETTER(AudioListenerHostObject, upZ));
}

// Explicit destructor acts as the "key function" so RTTI works across dynamic
// library boundaries (dynamic_cast in isHostObject) — Android-specific issue.
AudioListenerHostObject::~AudioListenerHostObject() = default;

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, positionX) {
  return jsi::Object::createFromHostObject(runtime, positionXParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, positionY) {
  return jsi::Object::createFromHostObject(runtime, positionYParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, positionZ) {
  return jsi::Object::createFromHostObject(runtime, positionZParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, forwardX) {
  return jsi::Object::createFromHostObject(runtime, forwardXParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, forwardY) {
  return jsi::Object::createFromHostObject(runtime, forwardYParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, forwardZ) {
  return jsi::Object::createFromHostObject(runtime, forwardZParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, upX) {
  return jsi::Object::createFromHostObject(runtime, upXParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, upY) {
  return jsi::Object::createFromHostObject(runtime, upYParam_);
}

JSI_PROPERTY_GETTER_IMPL(AudioListenerHostObject, upZ) {
  return jsi::Object::createFromHostObject(runtime, upZParam_);
}

} // namespace audioapi
