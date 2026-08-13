#pragma once

#include <audioapi/jsi/RuntimeObserver.h>
#include <jsi/jsi.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/decoding/AudioDecoding.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioArrayBuffer.hpp>

namespace audioapi::option_parser {

inline std::map<std::string, std::string> parseHttpHeaders(
    jsi::Runtime &runtime,
    const jsi::Object &headersObject) {
  std::map<std::string, std::string> headers;
  const auto propertyNames = headersObject.getPropertyNames(runtime);
  const auto count = propertyNames.size(runtime);
  for (size_t i = 0; i < count; ++i) {
    const auto keyValue = propertyNames.getValueAtIndex(runtime, i);
    if (!keyValue.isString()) {
      continue;
    }
    const auto key = keyValue.asString(runtime).utf8(runtime);
    const auto value = headersObject.getProperty(runtime, key.c_str());
    if (value.isString()) {
      headers.emplace(key, value.asString(runtime).utf8(runtime));
    }
  }
  return headers;
}

inline void mergeHttpHeaders(
    std::map<std::string, std::string> &target,
    const std::map<std::string, std::string> &incoming) {
  for (const auto &[key, value] : incoming) {
    target[key] = value;
  }
}

inline AudioNodeOptions parseAudioNodeOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  AudioNodeOptions options;

  auto channelCountValue = optionsObject.getProperty(runtime, "channelCount");
  if (channelCountValue.isNumber()) {
    options.channelCount = static_cast<int>(channelCountValue.getNumber());
  }

  auto channelCountModeValue = optionsObject.getProperty(runtime, "channelCountMode");
  if (channelCountModeValue.isString()) {
    auto channelCountModeStr = channelCountModeValue.asString(runtime).utf8(runtime);
    if (channelCountModeStr == "max") {
      options.channelCountMode = ChannelCountMode::MAX;
    } else if (channelCountModeStr == "clamped-max") {
      options.channelCountMode = ChannelCountMode::CLAMPED_MAX;
    } else if (channelCountModeStr == "explicit") {
      options.channelCountMode = ChannelCountMode::EXPLICIT;
    }
  }

  auto channelInterpretationValue = optionsObject.getProperty(runtime, "channelInterpretation");
  if (channelInterpretationValue.isString()) {
    auto channelInterpretationStr = channelInterpretationValue.asString(runtime).utf8(runtime);
    if (channelInterpretationStr == "speakers") {
      options.channelInterpretation = ChannelInterpretation::SPEAKERS;
    } else if (channelInterpretationStr == "discrete") {
      options.channelInterpretation = ChannelInterpretation::DISCRETE;
    }
  }

  return options;
}

inline GainOptions parseGainOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  GainOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto gainValue = optionsObject.getProperty(runtime, "gain");
  if (gainValue.isNumber()) {
    options.gain = static_cast<float>(gainValue.getNumber());
  }

  return options;
}

inline StereoPannerOptions parseStereoPannerOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  StereoPannerOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto panValue = optionsObject.getProperty(runtime, "pan");
  if (panValue.isNumber()) {
    options.pan = static_cast<float>(panValue.getNumber());
  }

  return options;
}

inline ConvolverOptions parseConvolverOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  ConvolverOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto disableNormalizationValue = optionsObject.getProperty(runtime, "disableNormalization");
  if (disableNormalizationValue.isBool()) {
    options.disableNormalization = disableNormalizationValue.getBool();
  }

  if (optionsObject.hasProperty(runtime, "buffer")) {
    auto bufferValue = optionsObject.getProperty(runtime, "buffer");
    if (bufferValue.isObject() &&
        bufferValue.getObject(runtime).isHostObject<AudioBufferHostObject>(runtime)) {
      options.buffer =
          bufferValue.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime)->audioBuffer_;
    }
  }
  return options;
}

inline ConstantSourceOptions parseConstantSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  ConstantSourceOptions options;

  auto offsetValue = optionsObject.getProperty(runtime, "offset");
  if (offsetValue.isNumber()) {
    options.offset = static_cast<float>(offsetValue.getNumber());
  }

  return options;
}

inline AnalyserOptions parseAnalyserOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  AnalyserOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto fftSizeValue = optionsObject.getProperty(runtime, "fftSize");
  if (fftSizeValue.isNumber()) {
    options.fftSize = static_cast<int>(fftSizeValue.getNumber());
  }

  auto minDecibelsValue = optionsObject.getProperty(runtime, "minDecibels");
  if (minDecibelsValue.isNumber()) {
    options.minDecibels = static_cast<float>(minDecibelsValue.getNumber());
  }

  auto maxDecibelsValue = optionsObject.getProperty(runtime, "maxDecibels");
  if (maxDecibelsValue.isNumber()) {
    options.maxDecibels = static_cast<float>(maxDecibelsValue.getNumber());
  }

  auto smoothingTimeConstantValue = optionsObject.getProperty(runtime, "smoothingTimeConstant");
  if (smoothingTimeConstantValue.isNumber()) {
    options.smoothingTimeConstant = static_cast<float>(smoothingTimeConstantValue.getNumber());
  }

  return options;
}

inline BiquadFilterOptions parseBiquadFilterOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  BiquadFilterOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto typeValue = optionsObject.getProperty(runtime, "type");
  if (typeValue.isString()) {
    auto typeStr = typeValue.asString(runtime).utf8(runtime);
    if (typeStr == "lowpass") {
      options.type = BiquadFilterType::LOWPASS;
    } else if (typeStr == "highpass") {
      options.type = BiquadFilterType::HIGHPASS;
    } else if (typeStr == "bandpass") {
      options.type = BiquadFilterType::BANDPASS;
    } else if (typeStr == "lowshelf") {
      options.type = BiquadFilterType::LOWSHELF;
    } else if (typeStr == "highshelf") {
      options.type = BiquadFilterType::HIGHSHELF;
    } else if (typeStr == "peaking") {
      options.type = BiquadFilterType::PEAKING;
    } else if (typeStr == "notch") {
      options.type = BiquadFilterType::NOTCH;
    } else if (typeStr == "allpass") {
      options.type = BiquadFilterType::ALLPASS;
    }
  }

  auto frequencyValue = optionsObject.getProperty(runtime, "frequency");
  if (frequencyValue.isNumber()) {
    options.frequency = static_cast<float>(frequencyValue.getNumber());
  }

  auto detuneValue = optionsObject.getProperty(runtime, "detune");
  if (detuneValue.isNumber()) {
    options.detune = static_cast<float>(detuneValue.getNumber());
  }

  auto QValue = optionsObject.getProperty(runtime, "Q");
  if (QValue.isNumber()) {
    options.Q = static_cast<float>(QValue.getNumber());
  }

  auto gainValue = optionsObject.getProperty(runtime, "gain");
  if (gainValue.isNumber()) {
    options.gain = static_cast<float>(gainValue.getNumber());
  }

  return options;
}

inline OscillatorOptions parseOscillatorOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  OscillatorOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto typeValue = optionsObject.getProperty(runtime, "type");
  if (typeValue.isString()) {
    auto typeStr = typeValue.asString(runtime).utf8(runtime);
    if (typeStr == "sine") {
      options.type = OscillatorType::SINE;
    } else if (typeStr == "square") {
      options.type = OscillatorType::SQUARE;
    } else if (typeStr == "sawtooth") {
      options.type = OscillatorType::SAWTOOTH;
    } else if (typeStr == "triangle") {
      options.type = OscillatorType::TRIANGLE;
    } else if (typeStr == "custom") {
      options.type = OscillatorType::CUSTOM;
    }
  }

  auto frequencyValue = optionsObject.getProperty(runtime, "frequency");
  if (frequencyValue.isNumber()) {
    options.frequency = static_cast<float>(frequencyValue.getNumber());
  }

  auto detuneValue = optionsObject.getProperty(runtime, "detune");
  if (detuneValue.isNumber()) {
    options.detune = static_cast<float>(detuneValue.getNumber());
  }

  auto periodicWaveValue = optionsObject.getProperty(runtime, "periodicWave");
  if (periodicWaveValue.isObject()) {
    auto periodicWaveHostObject =
        periodicWaveValue.getObject(runtime).asHostObject<PeriodicWaveHostObject>(runtime);
    options.periodicWave = periodicWaveHostObject->periodicWave_;
  }

  return options;
}

inline BaseAudioBufferSourceOptions parseBaseAudioBufferSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  BaseAudioBufferSourceOptions options;

  auto detuneValue = optionsObject.getProperty(runtime, "detune");
  if (detuneValue.isNumber()) {
    options.detune = static_cast<float>(detuneValue.getNumber());
  }

  auto playbackRateValue = optionsObject.getProperty(runtime, "playbackRate");
  if (playbackRateValue.isNumber()) {
    options.playbackRate = static_cast<float>(playbackRateValue.getNumber());
  }

  auto pitchCorrectionValue = optionsObject.getProperty(runtime, "pitchCorrection");
  if (pitchCorrectionValue.isBool()) {
    options.pitchCorrection = pitchCorrectionValue.getBool();
  }

  return options;
}

inline AudioBufferSourceOptions parseAudioBufferSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  AudioBufferSourceOptions options(parseBaseAudioBufferSourceOptions(runtime, optionsObject));

  if (optionsObject.hasProperty(runtime, "buffer")) {
    auto bufferValue = optionsObject.getProperty(runtime, "buffer");
    if (bufferValue.isObject() &&
        bufferValue.getObject(runtime).isHostObject<AudioBufferHostObject>(runtime)) {
      options.buffer =
          bufferValue.getObject(runtime).asHostObject<AudioBufferHostObject>(runtime)->audioBuffer_;
    }
  }

  auto loopValue = optionsObject.getProperty(runtime, "loop");
  if (loopValue.isBool()) {
    options.loop = loopValue.getBool();
  }

  auto loopStartValue = optionsObject.getProperty(runtime, "loopStart");
  if (loopStartValue.isNumber()) {
    options.loopStart = static_cast<float>(loopStartValue.getNumber());
  }

  auto loopEndValue = optionsObject.getProperty(runtime, "loopEnd");
  if (loopEndValue.isNumber()) {
    options.loopEnd = static_cast<float>(loopEndValue.getNumber());
  }

  return options;
}

inline AudioFileSourceOptions parseAudioFileSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  AudioFileSourceOptions options;

  auto nodeOpts = parseAudioNodeOptions(runtime, optionsObject);
  static_cast<AudioNodeOptions &>(options) = nodeOpts;
  options.numberOfInputs = 0;

  auto loopValue = optionsObject.getProperty(runtime, "loop");
  if (loopValue.isBool()) {
    options.loop = loopValue.getBool();
  }

  auto volumeValue = optionsObject.getProperty(runtime, "volume");
  if (volumeValue.isNumber()) {
    options.volume = static_cast<float>(volumeValue.getNumber());
  }

  auto playbackRateValue = optionsObject.getProperty(runtime, "playbackRate");
  if (playbackRateValue.isNumber()) {
    options.playbackRate = static_cast<float>(playbackRateValue.getNumber());
  }

  auto preservesPitchValue = optionsObject.getProperty(runtime, "preservesPitch");
  if (preservesPitchValue.isBool()) {
    options.preservesPitch = preservesPitchValue.getBool();
  }

  auto sourceValue = optionsObject.getProperty(runtime, "source");
  if (sourceValue.isString()) {
    const auto path = sourceValue.asString(runtime).utf8(runtime);
    if (audiodecoding::isHttpUrl(path)) {
      options.sourceUrl = path;
      // Remote progressive / HLS streaming uses FFmpeg's HTTP demuxer.
      options.requiresFFmpeg = true;
    } else {
      options.filePath = path;
      // Local files use OS/miniaudio — FFmpeg is only for remote/network sources.
      options.requiresFFmpeg = false;
    }
  } else if (sourceValue.isObject()) {
    auto sourceObj = sourceValue.asObject(runtime);
    if (sourceObj.hasProperty(runtime, "uri")) {
      const auto uriValue = sourceObj.getProperty(runtime, "uri");
      if (uriValue.isString()) {
        options.sourceUrl = uriValue.asString(runtime).utf8(runtime);
        options.requiresFFmpeg = true;
      }
      const auto sourceHeadersValue = sourceObj.getProperty(runtime, "headers");
      if (sourceHeadersValue.isObject()) {
        mergeHttpHeaders(
            options.httpHeaders, parseHttpHeaders(runtime, sourceHeadersValue.asObject(runtime)));
      }
    } else if (sourceObj.isArrayBuffer(runtime)) {
      auto arrayBuffer = sourceObj.getArrayBuffer(runtime);
      auto *data = arrayBuffer.data(runtime);
      auto size = arrayBuffer.size(runtime);
      options.data = std::vector<uint8_t>(data, data + size);
      // In-memory sources decode via OS / miniaudio — never FFmpeg.
      options.requiresFFmpeg = false;
    }
  }

  const auto headersValue = optionsObject.getProperty(runtime, "headers");
  if (headersValue.isObject()) {
    mergeHttpHeaders(
        options.httpHeaders, parseHttpHeaders(runtime, headersValue.asObject(runtime)));
  }

#if RN_AUDIO_API_FFMPEG_DISABLED
  if (options.requiresFFmpeg) {
    throw jsi::JSError(
        runtime,
        "AudioFileSourceNode: remote URL streaming and HLS (.m3u8) require FFmpeg, "
        "which is disabled in this build.");
  }
#endif

  return options;
}

inline DelayOptions parseDelayOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  DelayOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto maxDelayTimeValue = optionsObject.getProperty(runtime, "maxDelayTime");
  if (maxDelayTimeValue.isNumber()) {
    options.maxDelayTime = static_cast<float>(maxDelayTimeValue.getNumber());
  }

  auto delayTimeValue = optionsObject.getProperty(runtime, "delayTime");
  if (delayTimeValue.isNumber()) {
    options.delayTime = static_cast<float>(delayTimeValue.getNumber());
  }

  return options;
}

inline ChannelMergerOptions parseChannelMergerOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  // channelCount / channelCountMode are fixed by the spec and validated in TS.
  // channelInterpretation remains configurable.
  ChannelMergerOptions options;

  auto numberOfInputsValue = optionsObject.getProperty(runtime, "numberOfInputs");
  if (numberOfInputsValue.isNumber()) {
    options.numberOfInputs = static_cast<int>(numberOfInputsValue.getNumber());
  }

  auto channelInterpretationValue = optionsObject.getProperty(runtime, "channelInterpretation");
  if (!channelInterpretationValue.isString()) {
    return options;
  }
  auto channelInterpretationStr = channelInterpretationValue.asString(runtime).utf8(runtime);
  if (channelInterpretationStr == "speakers") {
    options.channelInterpretation = ChannelInterpretation::SPEAKERS;
  } else if (channelInterpretationStr == "discrete") {
    options.channelInterpretation = ChannelInterpretation::DISCRETE;
  }

  return options;
}

inline ChannelSplitterOptions parseChannelSplitterOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  // The Web Audio spec fixes channelCount/mode/interpretation for a splitter;
  // channelCount tracks numberOfOutputs.
  ChannelSplitterOptions options;

  auto numberOfOutputsValue = optionsObject.getProperty(runtime, "numberOfOutputs");
  if (numberOfOutputsValue.isNumber()) {
    options.numberOfOutputs = static_cast<int>(numberOfOutputsValue.getNumber());
    options.channelCount = options.numberOfOutputs;
  }

  return options;
}

inline IIRFilterOptions parseIIRFilterOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  IIRFilterOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto feedforwardValue = optionsObject.getProperty(runtime, "feedforward");
  if (feedforwardValue.isObject()) {
    auto feedforwardArray = feedforwardValue.asObject(runtime).asArray(runtime);
    size_t feedforwardLength = feedforwardArray.size(runtime);
    options.feedforward.reserve(feedforwardLength);
    for (size_t i = 0; i < feedforwardLength; ++i) {
      options.feedforward.push_back(
          static_cast<float>(feedforwardArray.getValueAtIndex(runtime, i).getNumber()));
    }
  }

  auto feedbackValue = optionsObject.getProperty(runtime, "feedback");
  if (feedbackValue.isObject()) {
    auto feedbackArray = feedbackValue.asObject(runtime).asArray(runtime);
    size_t feedbackLength = feedbackArray.size(runtime);
    options.feedback.reserve(feedbackLength);
    for (size_t i = 0; i < feedbackLength; ++i) {
      options.feedback.push_back(
          static_cast<float>(feedbackArray.getValueAtIndex(runtime, i).getNumber()));
    }
  }

  return options;
}

/// Parses Web IDL `sequence<float>` curve values: TypedArrays, JS arrays, and
/// other array-like objects with a numeric `length`.
inline std::shared_ptr<AudioArray> parseCurveSequence(
    jsi::Runtime &runtime,
    const jsi::Value &curveValue) {
  if (curveValue.isUndefined() || curveValue.isNull() || !curveValue.isObject()) {
    return nullptr;
  }

  auto curveObject = curveValue.getObject(runtime);

  if (curveObject.hasProperty(runtime, "buffer")) {
    auto arrayBuffer = curveObject.getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);
    const auto byteOffset =
        static_cast<size_t>(curveObject.getProperty(runtime, "byteOffset").getNumber());
    const auto byteLength =
        static_cast<size_t>(curveObject.getProperty(runtime, "byteLength").getNumber());
    const auto *data = reinterpret_cast<const float *>(
        static_cast<const uint8_t *>(arrayBuffer.data(runtime)) + byteOffset);

    const auto length = byteLength / sizeof(float);
    if (length < 2) {
      return nullptr;
    }

    return std::make_shared<AudioArray>(data, length);
  }

  size_t length = 0;
  if (curveObject.isArray(runtime)) {
    length = curveObject.asArray(runtime).size(runtime);
  } else if (curveObject.hasProperty(runtime, "length")) {
    const auto lengthValue = curveObject.getProperty(runtime, "length");
    if (!lengthValue.isNumber()) {
      return nullptr;
    }
    length = static_cast<size_t>(lengthValue.getNumber());
  } else {
    return nullptr;
  }

  if (length < 2) {
    return nullptr;
  }

  auto curve = std::make_shared<AudioArray>(length);
  auto data = curve->span();

  if (curveObject.isArray(runtime)) {
    const auto jsArray = curveObject.asArray(runtime);
    for (size_t i = 0; i < length; ++i) {
      const auto element = jsArray.getValueAtIndex(runtime, i);
      if (!element.isNumber()) {
        return nullptr;
      }
      data[i] = static_cast<float>(element.getNumber());
    }
    return curve;
  }

  for (size_t i = 0; i < length; ++i) {
    const std::string indexKey = std::to_string(i);
    const auto element = curveObject.getProperty(runtime, indexKey.c_str());
    if (!element.isNumber()) {
      return nullptr;
    }
    data[i] = static_cast<float>(element.getNumber());
  }
  return curve;
}

inline bool isFloat32Array(jsi::Runtime &runtime, const jsi::Object &object) {
  if (!object.hasProperty(runtime, "buffer")) {
    return false;
  }

  const auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  return object.instanceOf(runtime, float32ArrayCtor);
}

/// Creates a `Float32Array` attribute value from parsed curve samples.
inline jsi::Object createFloat32ArrayFromCurve(
    jsi::Runtime &runtime,
    const std::shared_ptr<AudioArray> &curve) {
  auto arrayBufferHandle = std::make_shared<AudioArrayBuffer>(curve->begin(), curve->getSize());
  auto arrayBuffer = jsi::ArrayBuffer(runtime, arrayBufferHandle);
  auto float32ArrayCtor = runtime.global().getPropertyAsFunction(runtime, "Float32Array");
  return float32ArrayCtor.callAsConstructor(runtime, arrayBuffer).getObject(runtime);
}

/// Converts a Web IDL `sequence<float>` constructor option into the `Float32Array`
/// stored on the `curve` attribute. Preserves the original `Float32Array` reference.
inline jsi::Value curveSequenceToAttributeValue(
    jsi::Runtime &runtime,
    const jsi::Value &sequenceValue,
    const std::shared_ptr<AudioArray> &parsedCurve) {
  if (sequenceValue.isObject() && isFloat32Array(runtime, sequenceValue.getObject(runtime))) {
    return jsi::Value(runtime, sequenceValue);
  }

  return createFloat32ArrayFromCurve(runtime, parsedCurve);
}

inline WaveShaperOptions parseWaveShaperOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  WaveShaperOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto oversampleValue = optionsObject.getProperty(runtime, "oversample");
  if (oversampleValue.isString()) {
    auto oversampleStr = oversampleValue.asString(runtime).utf8(runtime);
    if (oversampleStr == "none") {
      options.oversample = OverSampleType::OVERSAMPLE_NONE;
    } else if (oversampleStr == "2x") {
      options.oversample = OverSampleType::OVERSAMPLE_2X;
    } else if (oversampleStr == "4x") {
      options.oversample = OverSampleType::OVERSAMPLE_4X;
    }
  }

  const auto curveValue = optionsObject.getProperty(runtime, "curve");
  if (!curveValue.isUndefined() && !curveValue.isNull()) {
    options.curve = parseCurveSequence(runtime, curveValue);
    if (options.curve == nullptr) {
      throw jsi::JSError(
          runtime,
          "Failed to construct 'WaveShaperNode': The curve must contain at least two values.");
    }
  }

  return options;
}
} // namespace audioapi::option_parser
