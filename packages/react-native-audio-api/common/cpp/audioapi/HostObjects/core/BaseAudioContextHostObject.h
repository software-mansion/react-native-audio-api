#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>
#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/core/core/BaseAudioContext.h>
#include <audioapi/HostObjects/effects/BiquadFilterNodeHostObject.h>
#include <audioapi/HostObjects/effects/GainNodeHostObject.h>
#include <audioapi/HostObjects/sources/OscillatorNodeHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/HostObjects/effects/StereoPannerNodeHostObject.h>
#include <audioapi/HostObjects/analysis/AnalyserNodeHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/HostObjects/sources/StreamerNodeHostObject.h>

#include <jsi/jsi.h>
#include <memory>
#include <utility>
#include <vector>
#include <cstddef>

namespace audioapi {
using namespace facebook;

class BaseAudioContextHostObject : public JsiHostObject {
 public:
  explicit BaseAudioContextHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker)
      : context_(context), callInvoker_(callInvoker) {
      promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);

    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, destination),
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, state),
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, sampleRate),
        JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, currentTime));

    addFunctions(
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createRecorderAdapter),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createOscillator),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStreamer),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createGain),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStereoPanner),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBiquadFilter),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferSource),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferQueueSource),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBuffer),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createPeriodicWave),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createAnalyser),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, decodeAudioData),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, decodeAudioDataSource),
        JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, decodePCMAudioDataInBase64));
  }

  JSI_PROPERTY_GETTER_DECL(destination);
  JSI_PROPERTY_GETTER_DECL(state);
  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(currentTime);

  JSI_HOST_FUNCTION_DECL(createRecorderAdapter);
  JSI_HOST_FUNCTION_DECL(createOscillator);
  JSI_HOST_FUNCTION_DECL(createStreamer);
  JSI_HOST_FUNCTION_DECL(createGain);
  JSI_HOST_FUNCTION_DECL(createStereoPanner);
  JSI_HOST_FUNCTION_DECL(createBiquadFilter);
  JSI_HOST_FUNCTION_DECL(createBufferSource);
  JSI_HOST_FUNCTION_DECL(createBufferQueueSource);
  JSI_HOST_FUNCTION_DECL(createBuffer);
  JSI_HOST_FUNCTION_DECL(createPeriodicWave);
  JSI_HOST_FUNCTION_DECL(createAnalyser);
  JSI_HOST_FUNCTION_DECL(decodeAudioDataSource);
  JSI_HOST_FUNCTION_DECL(decodeAudioData);
  JSI_HOST_FUNCTION_DECL(decodePCMAudioDataInBase64);

  std::shared_ptr<BaseAudioContext> context_;

 protected:
  std::shared_ptr<PromiseVendor> promiseVendor_;
  std::shared_ptr<react::CallInvoker> callInvoker_;
};
} // namespace audioapi
