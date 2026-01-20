#include <audioapi/HostObjects/BaseAudioContextHostObject.h>

#include <audioapi/HostObjects/WorkletNodeHostObject.h>
#include <audioapi/HostObjects/WorkletProcessingNodeHostObject.h>
#include <audioapi/HostObjects/analysis/AnalyserNodeHostObject.h>
#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/HostObjects/effects/BiquadFilterNodeHostObject.h>
#include <audioapi/HostObjects/effects/ConvolverNodeHostObject.h>
#include <audioapi/HostObjects/effects/DelayNodeHostObject.h>
#include <audioapi/HostObjects/effects/GainNodeHostObject.h>
#include <audioapi/HostObjects/effects/IIRFilterNodeHostObject.h>
#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/HostObjects/effects/StereoPannerNodeHostObject.h>
#include <audioapi/HostObjects/effects/WaveShaperNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferQueueSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/OscillatorNodeHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/HostObjects/sources/StreamerNodeHostObject.h>
#include <audioapi/HostObjects/sources/WorkletSourceNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>

#include <audioapi/HostObjects/utils/NodeOptionsParser.h>
#include <memory>
#include <utility>
#include <vector>

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
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createWorkletProcessingNode),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createRecorderAdapter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createOscillator),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStreamer),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createConstantSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createGain),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createDelay),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStereoPanner),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBiquadFilter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createIIRFilter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferQueueSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBuffer),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createPeriodicWave),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createConvolver),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createAnalyser),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createWaveShaper));
}

// Explicitly define destructors here, as they to exist in order to act as a
// "key function" for the audio classes - this allow for RTTI to work
// properly across dynamic library boundaries (i.e. dynamic_cast that is used by
// isHostObject method), android specific issue
BaseAudioContextHostObject::~BaseAudioContextHostObject() = default;

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, destination) {
  auto destination = std::make_shared<AudioDestinationNodeHostObject>(context_->getDestination());
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
      worklets::extractSerializableOrThrow<worklets::SerializableWorklet>(runtime, args[0]);
  std::weak_ptr<worklets::WorkletRuntime> workletRuntime;
  auto shouldUseUiRuntime = args[1].getBool();
  auto shouldLockRuntime = shouldUseUiRuntime;
  if (shouldUseUiRuntime) {
    workletRuntime = context_->runtimeRegistry_.uiRuntime;
  } else {
    workletRuntime = context_->runtimeRegistry_.audioRuntime;
  }

  auto workletSourceNode =
      context_->createWorkletSourceNode(shareableWorklet, workletRuntime, shouldLockRuntime);
  auto workletSourceNodeHostObject =
      std::make_shared<WorkletSourceNodeHostObject>(workletSourceNode);
  return jsi::Object::createFromHostObject(runtime, workletSourceNodeHostObject);
#endif
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createWorkletNode) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  auto shareableWorklet =
      worklets::extractSerializableOrThrow<worklets::SerializableWorklet>(runtime, args[0]);

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
      shareableWorklet, workletRuntime, bufferLength, inputChannelCount, shouldLockRuntime);
  auto workletNodeHostObject = std::make_shared<WorkletNodeHostObject>(workletNode);
  auto jsiObject = jsi::Object::createFromHostObject(runtime, workletNodeHostObject);
  jsiObject.setExternalMemoryPressure(
      runtime,
      sizeof(float) * bufferLength * inputChannelCount); // rough estimate of underlying buffer
  return jsiObject;
#endif
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createWorkletProcessingNode) {
#if RN_AUDIO_API_ENABLE_WORKLETS
  auto shareableWorklet =
      worklets::extractSerializableOrThrow<worklets::SerializableWorklet>(runtime, args[0]);

  std::weak_ptr<worklets::WorkletRuntime> workletRuntime;
  auto shouldUseUiRuntime = args[1].getBool();
  auto shouldLockRuntime = shouldUseUiRuntime;
  if (shouldUseUiRuntime) {
    workletRuntime = context_->runtimeRegistry_.uiRuntime;
  } else {
    workletRuntime = context_->runtimeRegistry_.audioRuntime;
  }

  auto workletProcessingNode =
      context_->createWorkletProcessingNode(shareableWorklet, workletRuntime, shouldLockRuntime);
  auto workletProcessingNodeHostObject =
      std::make_shared<WorkletProcessingNodeHostObject>(workletProcessingNode);
  return jsi::Object::createFromHostObject(runtime, workletProcessingNodeHostObject);
#endif
  return jsi::Value::undefined();
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createRecorderAdapter) {
  auto recorderAdapter = context_->createRecorderAdapter();
  auto recorderAdapterHostObject = std::make_shared<RecorderAdapterNodeHostObject>(recorderAdapter);
  return jsi::Object::createFromHostObject(runtime, recorderAdapterHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createOscillator) {
  const auto options = args[0].asObject(runtime);
  const auto oscillatorOptions = audioapi::option_parser::parseOscillatorOptions(runtime, options);
  auto oscillator = context_->createOscillator(oscillatorOptions);
  auto oscillatorHostObject = std::make_shared<OscillatorNodeHostObject>(oscillator);
  return jsi::Object::createFromHostObject(runtime, oscillatorHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createStreamer) {
#if !RN_AUDIO_API_FFMPEG_DISABLED
  auto streamerOptions = StreamerOptions();
  if (!args[0].isUndefined()) {
    const auto options = args[0].asObject(runtime);
    streamerOptions = audioapi::option_parser::parseStreamerOptions(runtime, options);
  }
  auto streamer = context_->createStreamer(streamerOptions);
  auto streamerHostObject = std::make_shared<StreamerNodeHostObject>(streamer);
  auto object = jsi::Object::createFromHostObject(runtime, streamerHostObject);
  object.setExternalMemoryPressure(runtime, StreamerNodeHostObject::getSizeInBytes());
  return object;
#else
  return jsi::Value::undefined();
#endif // RN_AUDIO_API_FFMPEG_DISABLED
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createConstantSource) {
  const auto options = args[0].asObject(runtime);
  const auto constantSourceOptions =
      audioapi::option_parser::parseConstantSourceOptions(runtime, options);
  auto constantSource = context_->createConstantSource(constantSourceOptions);
  auto constantSourceHostObject = std::make_shared<ConstantSourceNodeHostObject>(constantSource);
  return jsi::Object::createFromHostObject(runtime, constantSourceHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createGain) {
  const auto options = args[0].asObject(runtime);
  const auto gainOptions = audioapi::option_parser::parseGainOptions(runtime, options);
  auto gain = context_->createGain(std::move(gainOptions));
  auto gainHostObject = std::make_shared<GainNodeHostObject>(gain);
  return jsi::Object::createFromHostObject(runtime, gainHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createDelay) {
  const auto options = args[0].asObject(runtime);
  const auto delayOptions = audioapi::option_parser::parseDelayOptions(runtime, options);
  auto delayNode = context_->createDelay(delayOptions);
  auto delayNodeHostObject = std::make_shared<DelayNodeHostObject>(delayNode);
  auto jsiObject = jsi::Object::createFromHostObject(runtime, delayNodeHostObject);
  jsiObject.setExternalMemoryPressure(runtime, delayNodeHostObject->getSizeInBytes());
  return jsiObject;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createStereoPanner) {
  const auto options = args[0].asObject(runtime);
  const auto stereoPannerOptions = audioapi::option_parser::parseStereoPannerOptions(runtime, options);
  auto stereoPanner = context_->createStereoPanner(stereoPannerOptions);
  auto stereoPannerHostObject = std::make_shared<StereoPannerNodeHostObject>(stereoPanner);
  return jsi::Object::createFromHostObject(runtime, stereoPannerHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBiquadFilter) {
  const auto options = args[0].asObject(runtime);
  const auto biquadFilterOptions = audioapi::option_parser::parseBiquadFilterOptions(runtime, options);
  auto biquadFilter = context_->createBiquadFilter(biquadFilterOptions);
  auto biquadFilterHostObject = std::make_shared<BiquadFilterNodeHostObject>(biquadFilter);
  return jsi::Object::createFromHostObject(runtime, biquadFilterHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createIIRFilter) {
  const auto options = args[0].asObject(runtime);
  const auto iirFilterOptions = audioapi::option_parser::parseIIRFilterOptions(runtime, options);
  auto iirFilter = context_->createIIRFilter(iirFilterOptions);
  auto iirFilterHostObject = std::make_shared<IIRFilterNodeHostObject>(iirFilter);
  return jsi::Object::createFromHostObject(runtime, iirFilterHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBufferSource) {
  const auto options = args[0].asObject(runtime);
  const auto audioBufferSourceOptions =
      audioapi::option_parser::parseAudioBufferSourceOptions(runtime, options);
  auto bufferSource = context_->createBufferSource(audioBufferSourceOptions);
  auto bufferSourceHostObject = std::make_shared<AudioBufferSourceNodeHostObject>(bufferSource);
  return jsi::Object::createFromHostObject(runtime, bufferSourceHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBufferQueueSource) {
  const auto options = args[0].asObject(runtime);
  const auto baseAudioBufferSourceOptions =
      audioapi::option_parser::parseBaseAudioBufferSourceOptions(runtime, options);
  auto bufferSource = context_->createBufferQueueSource(baseAudioBufferSourceOptions);
  auto bufferStreamSourceHostObject =
      std::make_shared<AudioBufferQueueSourceNodeHostObject>(bufferSource);
  return jsi::Object::createFromHostObject(runtime, bufferStreamSourceHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBuffer) {
  const auto options = args[0].asObject(runtime);
  const auto audioBufferOptions = audioapi::option_parser::parseAudioBufferOptions(runtime, options);
  auto buffer = BaseAudioContext::createBuffer(audioBufferOptions);
  auto bufferHostObject = std::make_shared<AudioBufferHostObject>(buffer);

  auto jsiObject = jsi::Object::createFromHostObject(runtime, bufferHostObject);
  jsiObject.setExternalMemoryPressure(runtime, bufferHostObject->getSizeInBytes());

  return jsiObject;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createPeriodicWave) {
  auto arrayBufferReal =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto real = reinterpret_cast<float *>(arrayBufferReal.data(runtime));
  auto length = static_cast<int>(arrayBufferReal.size(runtime));

  auto arrayBufferImag =
      args[1].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto imag = reinterpret_cast<float *>(arrayBufferImag.data(runtime));

  auto disableNormalization = args[2].getBool();

  auto complexData = std::vector<std::complex<float>>(length);

  for (size_t i = 0; i < length; i++) {
    complexData[i] = std::complex<float>(static_cast<float>(real[i]), static_cast<float>(imag[i]));
  }

  auto periodicWave = context_->createPeriodicWave(complexData, disableNormalization, length);
  auto periodicWaveHostObject = std::make_shared<PeriodicWaveHostObject>(periodicWave);

  return jsi::Object::createFromHostObject(runtime, periodicWaveHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createAnalyser) {
  const auto options = args[0].asObject(runtime);
  const auto analyserOptions = audioapi::option_parser::parseAnalyserOptions(runtime, options);
  auto analyser = context_->createAnalyser(analyserOptions);
  auto analyserHostObject = std::make_shared<AnalyserNodeHostObject>(analyser);
  return jsi::Object::createFromHostObject(runtime, analyserHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createConvolver) {
  const auto options = args[0].asObject(runtime);
  const auto convolverOptions = audioapi::option_parser::parseConvolverOptions(runtime, options);
  auto convolver = context_->createConvolver(convolverOptions);
  auto convolverHostObject = std::make_shared<ConvolverNodeHostObject>(convolver);
  auto jsiObject = jsi::Object::createFromHostObject(runtime, convolverHostObject);
  if (convolverOptions.bus != nullptr) {
    auto bufferHostObject = options.getProperty(runtime, "buffer").getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
    jsiObject.setExternalMemoryPressure(runtime, bufferHostObject->getSizeInBytes());
  }
  return jsiObject;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createWaveShaper) {
  const auto options = args[0].asObject(runtime);
  const auto waveShaperOptions = audioapi::option_parser::parseWaveShaperOptions(runtime, options);
  auto waveShaper = context_->createWaveShaper(waveShaperOptions);
  auto waveShaperHostObject = std::make_shared<WaveShaperNodeHostObject>(waveShaper);
  return jsi::Object::createFromHostObject(runtime, waveShaperHostObject);
}
} // namespace audioapi
