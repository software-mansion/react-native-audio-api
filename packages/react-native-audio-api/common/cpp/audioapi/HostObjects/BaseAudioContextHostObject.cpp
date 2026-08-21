#include <audioapi/HostObjects/AudioListenerHostObject.h>
#include <audioapi/HostObjects/BaseAudioContextHostObject.h>
#include <audioapi/HostObjects/analysis/AnalyserNodeHostObject.h>
#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>
#include <audioapi/HostObjects/effects/BiquadFilterNodeHostObject.h>
#include <audioapi/HostObjects/effects/ChannelMergerNodeHostObject.h>
#include <audioapi/HostObjects/effects/ChannelSplitterNodeHostObject.h>
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
#include <audioapi/HostObjects/sources/AudioFileSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/ConstantSourceNodeHostObject.h>
#include <audioapi/HostObjects/sources/OscillatorNodeHostObject.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/HostObjects/utils/JsEnumParser.h>
#include <audioapi/HostObjects/utils/NodeOptionsParser.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/decoding/AudioDecoding.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

BaseAudioContextHostObject::BaseAudioContextHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker,
    int destinationChannelCount)
    : context_(context),
      promiseVendor_(std::make_shared<PromiseVendor>(runtime, callInvoker)),
      callInvoker_(callInvoker) {
  destination_ = std::make_shared<AudioDestinationNodeHostObject>(
      context_, AudioDestinationOptions(destinationChannelCount));
  listener_ = std::make_shared<AudioListenerHostObject>(context_);

  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, destination),
      JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, listener),
      JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, sampleRate),
      JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, currentTime),
      JSI_EXPORT_PROPERTY_GETTER(BaseAudioContextHostObject, state));

  addSetters(JSI_EXPORT_PROPERTY_SETTER(BaseAudioContextHostObject, onstatechange));

  addFunctions(
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createRecorderAdapter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createOscillator),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createConstantSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createGain),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createDelay),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createStereoPanner),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBiquadFilter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createIIRFilter),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createFileSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createBufferQueueSource),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createPeriodicWave),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createConvolver),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createAnalyser),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createWaveShaper),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createChannelMerger),
      JSI_EXPORT_FUNCTION(BaseAudioContextHostObject, createChannelSplitter));
}

// Explicitly define destructors here, as they to exist in order to act as a
// "key function" for the audio classes - this allow for RTTI to work
// properly across dynamic library boundaries (i.e. dynamic_cast that is used by
// isHostObject method), android specific issue
BaseAudioContextHostObject::~BaseAudioContextHostObject() {
  // The C++ context can outlive this HostObject (lifecycle promises hold it);
  // never let it fire statechange into a GC'd JSI function.
  context_->assignOnStateChangeCallbackId(0);
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, state) {
  return jsi::String::createFromUtf8(runtime, contextStateToString(context_->getPublishedState()));
}

JSI_PROPERTY_SETTER_IMPL(BaseAudioContextHostObject, onstatechange) {
  context_->assignOnStateChangeCallbackId(std::stoull(value.getString(runtime).utf8(runtime)));
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, destination) {
  return jsi::Object::createFromHostObject(runtime, destination_);
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, listener) {
  return jsi::Object::createFromHostObject(runtime, listener_);
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, sampleRate) {
  return {context_->getSampleRate()};
}

JSI_PROPERTY_GETTER_IMPL(BaseAudioContextHostObject, currentTime) {
  return {context_->getCurrentTime()};
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createRecorderAdapter) {
  auto recorderAdapterHostObject = std::make_shared<RecorderAdapterNodeHostObject>(context_);
  auto object = jsi::Object::createFromHostObject(runtime, recorderAdapterHostObject);
  object.setExternalMemoryPressure(runtime, recorderAdapterHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createOscillator) {
  const auto options = args[0].asObject(runtime);
  const auto oscillatorOptions = audioapi::option_parser::parseOscillatorOptions(runtime, options);
  auto oscillatorHostObject =
      std::make_shared<OscillatorNodeHostObject>(context_, oscillatorOptions);
  auto object = jsi::Object::createFromHostObject(runtime, oscillatorHostObject);
  object.setExternalMemoryPressure(runtime, oscillatorHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createConstantSource) {
  const auto options = args[0].asObject(runtime);
  const auto constantSourceOptions =
      audioapi::option_parser::parseConstantSourceOptions(runtime, options);
  auto constantSourceHostObject =
      std::make_shared<ConstantSourceNodeHostObject>(context_, constantSourceOptions);
  auto object = jsi::Object::createFromHostObject(runtime, constantSourceHostObject);
  object.setExternalMemoryPressure(runtime, constantSourceHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createGain) {
  const auto options = args[0].asObject(runtime);
  const auto gainOptions = audioapi::option_parser::parseGainOptions(runtime, options);
  auto gainHostObject = std::make_shared<GainNodeHostObject>(context_, gainOptions);
  auto object = jsi::Object::createFromHostObject(runtime, gainHostObject);
  object.setExternalMemoryPressure(runtime, gainHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createDelay) {
  const auto options = args[0].asObject(runtime);
  const auto delayOptions = audioapi::option_parser::parseDelayOptions(runtime, options);
  auto delayNodeHostObject = std::make_shared<DelayNodeHostObject>(context_, delayOptions);
  auto jsiObject = jsi::Object::createFromHostObject(runtime, delayNodeHostObject);
  jsiObject.setExternalMemoryPressure(runtime, delayNodeHostObject->getMemoryPressure());
  return jsiObject;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createChannelMerger) {
  const auto options = args[0].asObject(runtime);
  const auto mergerOptions = audioapi::option_parser::parseChannelMergerOptions(runtime, options);
  if (mergerOptions.numberOfInputs < 1 || mergerOptions.numberOfInputs > MAX_CHANNEL_COUNT) {
    throw jsi::JSError(
        runtime,
        "IndexSizeError: numberOfInputs for ChannelMergerNode must be in the range [1, 32]");
  }
  auto mergerHostObject = std::make_shared<ChannelMergerNodeHostObject>(context_, mergerOptions);
  auto object = jsi::Object::createFromHostObject(runtime, mergerHostObject);
  object.setExternalMemoryPressure(runtime, mergerHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createChannelSplitter) {
  const auto options = args[0].asObject(runtime);
  const auto splitterOptions =
      audioapi::option_parser::parseChannelSplitterOptions(runtime, options);
  if (splitterOptions.numberOfOutputs < 1 || splitterOptions.numberOfOutputs > MAX_CHANNEL_COUNT) {
    throw jsi::JSError(
        runtime,
        "IndexSizeError: numberOfOutputs for ChannelSplitterNode must be in the range [1, 32]");
  }
  auto splitterHostObject =
      std::make_shared<ChannelSplitterNodeHostObject>(context_, splitterOptions);
  auto object = jsi::Object::createFromHostObject(runtime, splitterHostObject);
  object.setExternalMemoryPressure(runtime, splitterHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createStereoPanner) {
  const auto options = args[0].asObject(runtime);
  const auto stereoPannerOptions =
      audioapi::option_parser::parseStereoPannerOptions(runtime, options);
  auto stereoPannerHostObject =
      std::make_shared<StereoPannerNodeHostObject>(context_, stereoPannerOptions);
  auto object = jsi::Object::createFromHostObject(runtime, stereoPannerHostObject);
  object.setExternalMemoryPressure(runtime, stereoPannerHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBiquadFilter) {
  const auto options = args[0].asObject(runtime);
  const auto biquadFilterOptions =
      audioapi::option_parser::parseBiquadFilterOptions(runtime, options);
  auto biquadFilterHostObject =
      std::make_shared<BiquadFilterNodeHostObject>(context_, biquadFilterOptions);
  auto object = jsi::Object::createFromHostObject(runtime, biquadFilterHostObject);
  object.setExternalMemoryPressure(runtime, biquadFilterHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createIIRFilter) {
  const auto options = args[0].asObject(runtime);
  const auto iirFilterOptions = audioapi::option_parser::parseIIRFilterOptions(runtime, options);
  auto iirFilterHostObject = std::make_shared<IIRFilterNodeHostObject>(context_, iirFilterOptions);
  auto object = jsi::Object::createFromHostObject(runtime, iirFilterHostObject);
  object.setExternalMemoryPressure(runtime, iirFilterHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBufferSource) {
  const auto options = args[0].asObject(runtime);
  const auto audioBufferSourceOptions =
      audioapi::option_parser::parseAudioBufferSourceOptions(runtime, options);
  auto bufferSourceHostObject =
      std::make_shared<AudioBufferSourceNodeHostObject>(context_, audioBufferSourceOptions);
  auto object = jsi::Object::createFromHostObject(runtime, bufferSourceHostObject);
  object.setExternalMemoryPressure(runtime, bufferSourceHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createFileSource) {
  auto makeFileSourceHostObject = [&](AudioFileSourceOptions &opts) -> jsi::Value {
#if RN_AUDIO_API_FFMPEG_DISABLED
    if (opts.requiresFFmpeg) {
      return jsi::Value::undefined();
    }
#endif // RN_AUDIO_API_FFMPEG_DISABLED
    const auto fileSourceHostObject =
        std::make_shared<AudioFileSourceNodeHostObject>(context_, opts);
    auto object = jsi::Object::createFromHostObject(runtime, fileSourceHostObject);
    object.setExternalMemoryPressure(runtime, fileSourceHostObject->getMemoryPressure());
    return object;
  };

  const auto options = args[0].asObject(runtime);

  auto fileSourceOptions = audioapi::option_parser::parseAudioFileSourceOptions(runtime, options);
  return makeFileSourceHostObject(fileSourceOptions);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createBufferQueueSource) {
  const auto options = args[0].asObject(runtime);
  const auto baseAudioBufferSourceOptions =
      audioapi::option_parser::parseBaseAudioBufferSourceOptions(runtime, options);
  auto bufferStreamSourceHostObject = std::make_shared<AudioBufferQueueSourceNodeHostObject>(
      context_, baseAudioBufferSourceOptions);
  auto object = jsi::Object::createFromHostObject(runtime, bufferStreamSourceHostObject);
  object.setExternalMemoryPressure(runtime, bufferStreamSourceHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createPeriodicWave) {
  auto arrayBufferReal =
      args[0].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto *real = reinterpret_cast<float *>(arrayBufferReal.data(runtime));
  auto length = static_cast<int>(arrayBufferReal.size(runtime) / sizeof(float));

  auto arrayBufferImag =
      args[1].getObject(runtime).getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
  auto *imag = reinterpret_cast<float *>(arrayBufferImag.data(runtime));

  auto disableNormalization = args[2].getBool();

  auto complexData = std::vector<std::complex<float>>(length);

  for (int i = 0; i < length; i++) {
    complexData[i] = std::complex<float>(real[i], imag[i]);
  }

  auto periodicWave = context_->createPeriodicWave(complexData, disableNormalization, length);
  auto periodicWaveHostObject = std::make_shared<PeriodicWaveHostObject>(periodicWave);

  return jsi::Object::createFromHostObject(runtime, periodicWaveHostObject);
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createAnalyser) {
  const auto options = args[0].asObject(runtime);
  const auto analyserOptions = audioapi::option_parser::parseAnalyserOptions(runtime, options);
  auto analyserHostObject = std::make_shared<AnalyserNodeHostObject>(context_, analyserOptions);
  auto object = jsi::Object::createFromHostObject(runtime, analyserHostObject);
  object.setExternalMemoryPressure(runtime, analyserHostObject->getMemoryPressure());
  return object;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createConvolver) {
  const auto options = args[0].asObject(runtime);
  const auto convolverOptions = audioapi::option_parser::parseConvolverOptions(runtime, options);
  auto convolverHostObject = std::make_shared<ConvolverNodeHostObject>(context_, convolverOptions);
  auto jsiObject = jsi::Object::createFromHostObject(runtime, convolverHostObject);
  // getMemoryPressure() already folds in the IR buffer if one was passed
  // via options (the HO tracks `irBytes_` when `setBuffer` runs from its ctor).
  jsiObject.setExternalMemoryPressure(runtime, convolverHostObject->getMemoryPressure());
  return jsiObject;
}

JSI_HOST_FUNCTION_IMPL(BaseAudioContextHostObject, createWaveShaper) {
  const auto options = args[0].asObject(runtime);
  const auto waveShaperOptions = audioapi::option_parser::parseWaveShaperOptions(runtime, options);
  auto waveShaperHostObject =
      std::make_shared<WaveShaperNodeHostObject>(context_, waveShaperOptions);
  auto object = jsi::Object::createFromHostObject(runtime, waveShaperHostObject);
  object.setExternalMemoryPressure(runtime, waveShaperHostObject->getMemoryPressure());
  return object;
}
} // namespace audioapi
