#include <thread>

#include "BaseAudioContextHostObject.h"

namespace audioapi {
using namespace facebook;

BaseAudioContextHostObject::BaseAudioContextHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const std::shared_ptr<JsiPromise::PromiseVendor> &promiseVendor)
    : context_(context), promiseVendor_(promiseVendor) {
  destination_ = std::make_shared<AudioDestinationNodeHostObject>(context->getDestination());
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
  propertyNames.push_back(
      jsi::PropNameID::forUtf8(runtime, "decodeAudioDataSource"));
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
    return jsi::String::createFromUtf8(runtime, context_->getState());
  }

  if (propName == "sampleRate") {
    return {context_->getSampleRate()};
  }

  if (propName == "currentTime") {
    return {context_->getCurrentTime()};
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
          auto oscillator = context_->createOscillator();
          auto oscillatorHostObject = std::make_shared<OscillatorNodeHostObject>(oscillator);
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
          auto gain = context_->createGain();
          auto gainHostObject = std::make_shared<GainNodeHostObject>(gain);
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
          auto stereoPanner = context_->createStereoPanner();
          auto stereoPannerHostObject = std::make_shared<StereoPannerNodeHostObject>(stereoPanner);
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
          auto biquadFilter = context_->createBiquadFilter();
          auto biquadFilterHostObject = std::make_shared<BiquadFilterNodeHostObject>(biquadFilter);
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
          auto bufferSource = context_->createBufferSource();
          auto bufferSourceHostObject = std::make_shared<AudioBufferSourceNodeHostObject>(bufferSource);
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
                  context_->createBuffer(numberOfChannels, length, sampleRate);
          auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);
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

          auto periodicWave = context_->createPeriodicWave(
              realData, imagData, disableNormalization, length);
          auto periodicWaveHostObject = std::make_shared<PeriodicWaveHostObject>(periodicWave);
          return jsi::Object::createFromHostObject(
              runtime, periodicWaveHostObject);
        });
  }

  if (propName == "decodeAudioDataSource") {
    auto decode = [this](
                      jsi::Runtime &runtime,
                      const jsi::Value &,
                      const jsi::Value *arguments,
                      size_t count) -> jsi::Value {
      auto sourcePath = arguments[0].getString(runtime).utf8(runtime);

      auto promise = promiseVendor_->createPromise(
          [this, &runtime, sourcePath](
              std::shared_ptr<JsiPromise::Promise> promise) {
            std::thread([this,
                         &runtime,
                         sourcePath,
                         promise = std::move(promise)]() {
              auto results = context_->decodeAudioDataSource(sourcePath);
              auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(results);

              promise->resolve(jsi::Object::createFromHostObject(
                  runtime, audioBufferHostObject));
            }).detach();
          });

      return promise;
    };

    return jsi::Function::createFromHostFunction(
        runtime, propNameId, 1, decode);
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
