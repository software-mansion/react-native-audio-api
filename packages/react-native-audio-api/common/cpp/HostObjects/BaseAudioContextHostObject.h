#pragma once

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>

#include <JsiHostObject.h>
#include <JsiPromise.h>

#include "AudioBufferHostObject.h"
#include "AudioBufferSourceNodeHostObject.h"
#include "AudioDestinationNodeHostObject.h"
#include "BaseAudioContext.h"
#include "BiquadFilterNodeHostObject.h"
#include "GainNodeHostObject.h"
#include "OscillatorNodeHostObject.h"
#include "PeriodicWaveHostObject.h"
#include "StereoPannerNodeHostObject.h"
#include "AnalyserNodeHostObject.h"

namespace audioapi {
using namespace facebook;

class BaseAudioContextHostObject : public JsiHostObject {
 public:
  explicit BaseAudioContextHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const std::shared_ptr<PromiseVendor> &promiseVendor)
      : context_(context), promiseVendor_(promiseVendor) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, destination),
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, state),
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, sampleRate),
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, currentTime));

    addFunctions(
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createOscillator),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createGain),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStereoPanner),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBiquadFilter),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferSource),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBuffer),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createPeriodicWave),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createAnalyser),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, decodeAudioDataSource));
  }

  JSI_PROPERTY_GETTER(destination) {
    auto destination = std::make_shared<AudioDestinationNodeHostObject>(
        context_->getDestination());
    return jsi::Object::createFromHostObject(runtime, destination);
  }

  JSI_PROPERTY_GETTER(state) {
    return jsi::String::createFromUtf8(runtime, context_->getState());
  }

  JSI_PROPERTY_GETTER(sampleRate) {
    return {context_->getSampleRate()};
  }

  JSI_PROPERTY_GETTER(currentTime) {
    return {context_->getCurrentTime()};
  }

  JSI_HOST_FUNCTION(createOscillator) {
    auto oscillator = context_->createOscillator();
    auto oscillatorHostObject =
        std::make_shared<OscillatorNodeHostObject>(oscillator);
    return jsi::Object::createFromHostObject(runtime, oscillatorHostObject);
  }

  JSI_HOST_FUNCTION(createGain) {
    auto gain = context_->createGain();
    auto gainHostObject = std::make_shared<GainNodeHostObject>(gain);
    return jsi::Object::createFromHostObject(runtime, gainHostObject);
  }

  JSI_HOST_FUNCTION(createStereoPanner) {
    auto stereoPanner = context_->createStereoPanner();
    auto stereoPannerHostObject =
        std::make_shared<StereoPannerNodeHostObject>(stereoPanner);
    return jsi::Object::createFromHostObject(runtime, stereoPannerHostObject);
  }

  JSI_HOST_FUNCTION(createBiquadFilter) {
    auto biquadFilter = context_->createBiquadFilter();
    auto biquadFilterHostObject =
        std::make_shared<BiquadFilterNodeHostObject>(biquadFilter);
    return jsi::Object::createFromHostObject(runtime, biquadFilterHostObject);
  }

  JSI_HOST_FUNCTION(createBufferSource) {
    auto bufferSource = context_->createBufferSource();
    auto bufferSourceHostObject =
        std::make_shared<AudioBufferSourceNodeHostObject>(bufferSource);
    return jsi::Object::createFromHostObject(runtime, bufferSourceHostObject);
  }

  JSI_HOST_FUNCTION(createBuffer) {
    auto numberOfChannels = static_cast<int>(args[0].getNumber());
    auto length = static_cast<int>(args[1].getNumber());
    auto sampleRate = static_cast<int>(args[2].getNumber());
    auto buffer = context_->createBuffer(numberOfChannels, length, sampleRate);
    auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);
    return jsi::Object::createFromHostObject(runtime, bufferHostObject);
  }

  JSI_HOST_FUNCTION(createPeriodicWave) {
    auto real = args[0].getObject(runtime).getArray(runtime);
    auto imag = args[1].getObject(runtime).getArray(runtime);
    auto disableNormalization = args[2].getBool();
    auto length =
        static_cast<int>(real.getProperty(runtime, "length").asNumber());

    auto *realData = new float[length];
    auto *imagData = new float[length];

    for (size_t i = 0; i < real.length(runtime); i++) {
      realData[i] =
          static_cast<float>(real.getValueAtIndex(runtime, i).getNumber());
    }
    for (size_t i = 0; i < imag.length(runtime); i++) {
      realData[i] =
          static_cast<float>(imag.getValueAtIndex(runtime, i).getNumber());
    }

    auto periodicWave = context_->createPeriodicWave(
        realData, imagData, disableNormalization, length);
    auto periodicWaveHostObject =
        std::make_shared<PeriodicWaveHostObject>(periodicWave);
    return jsi::Object::createFromHostObject(runtime, periodicWaveHostObject);
  }

  JSI_HOST_FUNCTION(createAnalyser) {
    auto analyser = context_->createAnalyser();
    auto analyserHostObject = std::make_shared<AnalyserNodeHostObject>(analyser);
    return jsi::Object::createFromHostObject(runtime, analyserHostObject);
  }

  JSI_HOST_FUNCTION(decodeAudioDataSource) {
    auto sourcePath = args[0].getString(runtime).utf8(runtime);

    auto promise = promiseVendor_->createPromise(
        [this, &runtime, sourcePath](std::shared_ptr<Promise> promise) {
          std::thread([this,
                       &runtime,
                       sourcePath,
                       promise = std::move(promise)]() {
            auto results = context_->decodeAudioDataSource(sourcePath);
            auto audioBufferHostObject =
                std::make_shared<AudioBufferHostObject>(results);

            promise->resolve(jsi::Object::createFromHostObject(
                runtime, audioBufferHostObject));
          }).detach();
        });

    return promise;
  }

 protected:
  std::shared_ptr<BaseAudioContext> context_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
};
} // namespace audioapi
