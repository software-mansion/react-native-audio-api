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
  propertyNames.push_back(
      jsi::PropNameID::forUtf8(runtime, "createPeriodicWave"));
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

  if (propName == "createPeriodicWave") {
    return jsi::Function::createFromHostFunction(
        runtime,
        propNameId,
        3,
        [this](
            jsi::Runtime &runtime,
            const jsi::Value &thisValue,
            const jsi::Value *arguments,
            size_t count) -> jsi::Value {
          auto real = arguments[0].getObject(runtime).getArray(runtime);
          auto imag = arguments[1].getObject(runtime).getArray(runtime);
          auto disableNormalization = arguments[2].getBool();
          auto length =
              static_cast<int>(real.getProperty(runtime, "length").asNumber());

          auto *realData = new float[length];
          auto *imagData = new float[length];

          for (size_t i = 0; i < real.length(runtime); i++) {
            realData[i] = static_cast<float>(
                real.getValueAtIndex(runtime, i).getNumber());
          }
          for (size_t i = 0; i < imag.length(runtime); i++) {
            realData[i] = static_cast<float>(
                imag.getValueAtIndex(runtime, i).getNumber());
          }

          auto periodicWave = wrapper_->createPeriodicWave(
              realData, imagData, disableNormalization, length);
          auto periodicWaveHostObject =
              PeriodicWaveHostObject::createFromWrapper(periodicWave);
          return jsi::Object::createFromHostObject(
              runtime, periodicWaveHostObject);
        });

    if (propName == "decodeAudioData") {
      return jsi::Function::createFromHostFunction(
          runtime,
          propNameId,
          1,
          [this](
              jsi::Runtime &runtime,
              const jsi::Value &thisValue,
              const jsi::Value *arguments,
              size_t count) -> jsi::Value {
            auto data = arguments[0].getObject(runtime).getArray(runtime);
            auto length = static_cast<int>(
                data.getProperty(runtime, "length").asNumber());

            auto audioData = new uint8_t[length];

            for (size_t i = 0; i < length; i++) {
              audioData[i] = static_cast<uint8_t>(
                  data.getValueAtIndex(runtime, i).getNumber());
            }

            auto buffer = wrapper_->decodeAudioData(audioData, length);
            auto audioBufferHostObject =
                AudioBufferHostObject::createFromWrapper(buffer);
            return jsi::Object::createFromHostObject(
                runtime, audioBufferHostObject);
          });
    }
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
