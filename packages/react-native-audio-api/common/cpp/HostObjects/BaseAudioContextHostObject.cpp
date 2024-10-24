#include "BaseAudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

BaseAudioContextHostObject::BaseAudioContextHostObject(
    const std::shared_ptr<BaseAudioContextWrapper> &wrapper)
    : wrapper_(wrapper) {
  auto destinationNodeWrapper = wrapper_->getDestination();
  destination_ =
      AudioDestinationNodeHostObject::createFromWrapper(destinationNodeWrapper);
}

std::vector<jsi::PropNameID> BaseAudioContextHostObject::getPropertyNames(
    jsi::Runtime &runtime) {
  std::vector<jsi::PropNameID> propertyNames;
  propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "destination"));
  propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "state"));
  propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "sampleRate"));
  propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "currentTime"));
  propertyNames.push_back(
      jsi::PropNameID::forUtf8(runtime, "createOscillator"));
  propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createGain"));
  propertyNames.push_back(
      jsi::PropNameID::forUtf8(runtime, "createStereoPanner"));
  propertyNames.push_back(
      jsi::PropNameID::forUtf8(runtime, "createBiquadFilter"));
  propertyNames.push_back(
      jsi::PropNameID::forUtf8(runtime, "createBufferSource"));
  propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createBuffer"));
  return propertyNames;
}

jsi::Value BaseAudioContextHostObject::get(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId) {
  auto propName = propNameId.utf8(runtime);

  if (propName == "destination") {
    return jsi::Object::createFromHostObject(runtime, destination_);
  }

  if (propName == "state") {
    return jsi::String::createFromUtf8(runtime, wrapper_->getState());
  }

  if (propName == "sampleRate") {
    return {wrapper_->getSampleRate()};
  }

  if (propName == "currentTime") {
    return {wrapper_->getCurrentTime()};
  }

  if (propName == "createOscillator") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        0,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto oscillator = wrapper_->createOscillator();
          auto oscillatorHostObject =
              OscillatorNodeHostObject::createFromWrapper(oscillator);
          return jsi::Object::createFromHostObject(
              runtime, oscillatorHostObject);
        });
  }

  if (propName == "createGain") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        0,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto gain = wrapper_->createGain();
          auto gainHostObject = GainNodeHostObject::createFromWrapper(gain);
          return jsi::Object::createFromHostObject(runtime, gainHostObject);
        });
  }

  if (propName == "createStereoPanner") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        0,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto stereoPanner = wrapper_->createStereoPanner();
          auto stereoPannerHostObject =
              StereoPannerNodeHostObject::createFromWrapper(stereoPanner);
          return jsi::Object::createFromHostObject(
              runtime, stereoPannerHostObject);
        });
  }

  if (propName == "createBiquadFilter") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        0,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto biquadFilter = wrapper_->createBiquadFilter();
          auto biquadFilterHostObject =
              BiquadFilterNodeHostObject::createFromWrapper(biquadFilter);
          return jsi::Object::createFromHostObject(
              runtime, biquadFilterHostObject);
        });
  }

  if (propName == "createBufferSource") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        0,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto bufferSource = wrapper_->createBufferSource();
          auto bufferSourceHostObject =
              AudioBufferSourceNodeHostObject::createFromWrapper(bufferSource);
          return jsi::Object::createFromHostObject(
              runtime, bufferSourceHostObject);
        });
  }

  if (propName == "createBuffer") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        3,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto numberOfChannels = static_cast<int>(arguments[0].getNumber());
          auto length = static_cast<int>(arguments[1].getNumber());
          auto sampleRate = static_cast<int>(arguments[2].getNumber());
          auto buffer =
              wrapper_->createBuffer(numberOfChannels, length, sampleRate);
          auto bufferHostObject =
              AudioBufferHostObject::createFromWrapper(buffer);
          return jsi::Object::createFromHostObject(runtime, bufferHostObject);
        });
  }

  throw std::runtime_error("Not yet implemented!");
}

void BaseAudioContextHostObject::set(
    jsi::Runtime &runtime,
    const jsi::PropNameID &propNameId,
    const jsi::Value &value) {
  auto propName = propNameId.utf8(runtime);

  throw std::runtime_error("Not yet implemented!");
}
} // namespace audioapi
