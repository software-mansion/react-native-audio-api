#include <audioapi/HostObjects/BaseAudioContextHostObject.h>

#include <audioapi/HostObjects/WorkletNodeHostObject.h>
#include <audioapi/HostObjects/WorkletProcessingNodeHostObject.h>
#include <audioapi/HostObjects/analysis/AnalyserNodeHostObject.h>
#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/HostObjects/effects/BiquadFilterNodeHostObject.h>
#include <audioapi/HostObjects/effects/ConvolverNodeHostObject.h>
#include <audioapi/HostObjects/effects/GainNodeHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/HostObjects/effects/StereoPannerNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/OscillatorNodeHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/HostObjects/sources/StreamerNodeHostObject.h>
#include <audioapi/HostObjects/sources/WorkletSourceNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>

namespace audioapi {

BaseAudioContextHostObject::BaseAudioContextHostObject(
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
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createWorkletSourceNode),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createWorkletNode),
      JSI_EXPORT_FUNCTION(
          BaseAudioContextHostObject, createWorkletProcessingNode),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createRecorderAdapter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createOscillator),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStreamer),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createConstantSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createGain),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStereoPanner),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBiquadFilter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferQueueSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBuffer),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createPeriodicWave),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createConvolver),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createAnalyser));
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, destination) {
  auto destination = std::make_shared<AudioDestinationNodeHostObject>(
      context_->getDestination());
  return jsi::Object::createFromHostObject(runtime, destination);
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, state) {
  return jsi::String::createFromUtf8(runtime, context_->getState());
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, sampleRate) {
  return {context_->getSampleRate()};
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, currentTime) {
  return {context_->getCurrentTime()};
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createWorkletSourceNode) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  auto shareableWorklet =
      worklets::extractSerializableOrThrow<worklets::SerializableWorklet>(
          runtime, args[0]);
  std::weak_ptr<worklets::WorkletRuntime> workletRuntime;
  auto shouldUseUiRuntime = args[1].getBool();
  auto shouldLockRuntime = shouldUseUiRuntime;
  if (shouldUseUiRuntime) {
    workletRuntime = context_->runtimeRegistry_.uiRuntime;
  } else {
    workletRuntime = context_->runtimeRegistry_.audioRuntime;
  }

  auto workletSourceNode = context_->createWorkletSourceNode(
      shareableWorklet, workletRuntime, shouldLockRuntime);
  auto workletSourceNodeHostObject =
      std::make_shared<WorkletSourceNodeHostObject>(workletSourceNode);
  return jsi::Object::createFromHostObject(
      runtime, workletSourceNodeHostObject);
#endif
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createWorkletNode) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  auto shareableWorklet =
      worklets::extractSerializableOrThrow<worklets::SerializableWorklet>(
          runtime, args[0]);

  std::weak_ptr<worklets::WorkletRuntime> workletRuntime;
  auto shouldUseUiRuntime = args[1].getBool();
  auto shouldLockRuntime = shouldUseUiRuntime;
  if (shouldUseUiRuntime) {
    workletRuntime = context_->runtimeRegistry_.uiRuntime;
  } else {
    workletRuntime = context_->runtimeRegistry_.audioRuntime;
  }
  auto bufferLength = static_cast<size_t>(args[2].getNumber());
  auto inputChannelCount = static_cast<size_t>(args[3].getNumber());

  auto workletNode = context_->createWorkletNode(
      shareableWorklet,
      workletRuntime,
      bufferLength,
      inputChannelCount,
      shouldLockRuntime);
  auto workletNodeHostObject =
      std::make_shared<WorkletNodeHostObject>(workletNode);
  return jsi::Object::createFromHostObject(runtime, workletNodeHostObject);
#endif
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(
    BaseAudioContextHostObject,
    createWorkletProcessingNode) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  auto shareableWorklet =
      worklets::extractSerializableOrThrow<worklets::SerializableWorklet>(
          runtime, args[0]);

  std::weak_ptr<worklets::WorkletRuntime> workletRuntime;
  auto shouldUseUiRuntime = args[1].getBool();
  auto shouldLockRuntime = shouldUseUiRuntime;
  if (shouldUseUiRuntime) {
    workletRuntime = context_->runtimeRegistry_.uiRuntime;
  } else {
    workletRuntime = context_->runtimeRegistry_.audioRuntime;
  }

  auto workletProcessingNode = context_->createWorkletProcessingNode(
      shareableWorklet, workletRuntime, shouldLockRuntime);
  auto workletProcessingNodeHostObject =
      std::make_shared<WorkletProcessingNodeHostObject>(workletProcessingNode);
  return jsi::Object::createFromHostObject(
      runtime, workletProcessingNodeHostObject);
#endif
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createRecorderAdapter) {
  auto recorderAdapter = context_->createRecorderAdapter();
  auto recorderAdapterHostObject =
      std::make_shared<RecorderAdapterNodeHostObject>(recorderAdapter);
  return jsi::Object::createFromHostObject(runtime, recorderAdapterHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createOscillator) {
  auto oscillator = context_->createOscillator();
  auto oscillatorHostObject =
      std::make_shared<OscillatorNodeHostObject>(oscillator);
  return jsi::Object::createFromHostObject(runtime, oscillatorHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createStreamer) {
  auto streamer = context_->createStreamer();
  auto streamerHostObject = std::make_shared<StreamerNodeHostObject>(streamer);
  auto object = jsi::Object::createFromHostObject(runtime, streamerHostObject);
  object.setExternalMemoryPressure(
      runtime, StreamerNodeHostObject::getSizeInBytes());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createConstantSource) {
  auto constantSource = context_->createConstantSource();
  auto constantSourceHostObject =
      std::make_shared<ConstantSourceNodeHostObject>(constantSource);
  return jsi::Object::createFromHostObject(runtime, constantSourceHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createGain) {
  auto gain = context_->createGain();
  auto gainHostObject = std::make_shared<GainNodeHostObject>(gain);
  return jsi::Object::createFromHostObject(runtime, gainHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createStereoPanner) {
  auto stereoPanner = context_->createStereoPanner();
  auto stereoPannerHostObject =
      std::make_shared<StereoPannerNodeHostObject>(stereoPanner);
  return jsi::Object::createFromHostObject(runtime, stereoPannerHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBiquadFilter) {
  auto biquadFilter = context_->createBiquadFilter();
  auto biquadFilterHostObject =
      std::make_shared<BiquadFilterNodeHostObject>(biquadFilter);
  return jsi::Object::createFromHostObject(runtime, biquadFilterHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBufferSource) {
  auto pitchCorrection = args[0].asBool();
  auto bufferSource = context_->createBufferSource(pitchCorrection);
  auto bufferSourceHostObject =
      std::make_shared<AudioBufferSourceNodeHostObject>(bufferSource);
  return jsi::Object::createFromHostObject(runtime, bufferSourceHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBufferQueueSource) {
  auto pitchCorrection = args[0].asBool();
  auto bufferSource = context_->createBufferQueueSource(pitchCorrection);
  auto bufferStreamSourceHostObject =
      std::make_shared<AudioBufferQueueSourceNodeHostObject>(bufferSource);
  return jsi::Object::createFromHostObject(
      runtime, bufferStreamSourceHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBuffer) {
  auto numberOfChannels = static_cast<int>(args[0].getNumber());
  auto length = static_cast<size_t>(args[1].getNumber());
  auto sampleRate = static_cast<float>(args[2].getNumber());
  auto buffer =
      BaseAudioContext::createBuffer(numberOfChannels, length, sampleRate);
  auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);

  auto jsiObject = jsi::Object::createFromHostObject(runtime, bufferHostObject);
  jsiObject.setExternalMemoryPressure(
      runtime, bufferHostObject->getSizeInBytes());

  return jsiObject;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createPeriodicWave) {
  auto arrayBufferReal = args[0]
                             .getObject(runtime)
                             .getPropertyAsObject(runtime, "buffer")
                             .getArrayBuffer(runtime);
  auto real = reinterpret_cast<float *>(arrayBufferReal.data(runtime));
  auto length = static_cast<int>(arrayBufferReal.size(runtime));

  auto arrayBufferImag = args[1]
                             .getObject(runtime)
                             .getPropertyAsObject(runtime, "buffer")
                             .getArrayBuffer(runtime);
  auto imag = reinterpret_cast<float *>(arrayBufferImag.data(runtime));

  auto disableNormalization = args[2].getBool();

  auto complexData = std::vector<std::complex<float>>(length);

  for (size_t i = 0; i < length; i++) {
    complexData[i] = std::complex<float>(
        static_cast<float>(real[i]), static_cast<float>(imag[i]));
  }

  auto periodicWave =
      context_->createPeriodicWave(complexData, disableNormalization, length);
  auto periodicWaveHostObject =
      std::make_shared<PeriodicWaveHostObject>(periodicWave);

  return jsi::Object::createFromHostObject(runtime, periodicWaveHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createAnalyser) {
  auto analyser = context_->createAnalyser();
  auto analyserHostObject = std::make_shared<AnalyserNodeHostObject>(analyser);
  return jsi::Object::createFromHostObject(runtime, analyserHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createConvolver) {
  auto disableNormalization = args[1].getBool();
  std::shared_ptr<ConvolverNode> convolver;
  if (args[0].isUndefined()) {
    convolver = context_->createConvolver(nullptr, disableNormalization);
  } else {
    auto bufferHostObject =
        args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    convolver = context_->createConvolver(
        bufferHostObject->audioBuffer_, disableNormalization);
  }
  auto convolverHostObject =
      std::make_shared<ConvolverNodeHostObject>(convolver);
  return jsi::Object::createFromHostObject(runtime, convolverHostObject);
}
} // namespace audioapi
