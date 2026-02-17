#pragma once

#include <audioapi/jsi/RuntimeLifecycleMonitor.h>
#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/types/NodeOptions.h>

namespace audioapi::option_parser {
AudioNodeOptions parseAudioNodeOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
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

GainOptions parseGainOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  GainOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto gainValue = optionsObject.getProperty(runtime, "gain");
  if (gainValue.isNumber()) {
    options.gain = static_cast<float>(gainValue.getNumber());
  }

  return options;
}

StereoPannerOptions parseStereoPannerOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  StereoPannerOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto panValue = optionsObject.getProperty(runtime, "pan");
  if (panValue.isNumber()) {
    options.pan = static_cast<float>(panValue.getNumber());
  }

  return options;
}

ConvolverOptions parseConvolverOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  ConvolverOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto disableNormalizationValue = optionsObject.getProperty(runtime, "disableNormalization");
  if (disableNormalizationValue.isBool()) {
    options.disableNormalization = disableNormalizationValue.getBool();
  }

  if (optionsObject.hasProperty(runtime, "buffer")) {
    auto bufferHostObject = optionsObject.getProperty(runtime, "buffer")
                                .getObject(runtime)
                                .asHostObject<AudioBufferHostObject>(runtime);
    options.buffer = bufferHostObject->audioBuffer_;
  }
  return options;
}

ConstantSourceOptions parseConstantSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  ConstantSourceOptions options;

  auto offsetValue = optionsObject.getProperty(runtime, "offset");
  if (offsetValue.isNumber()) {
    options.offset = static_cast<float>(offsetValue.getNumber());
  }

  return options;
}

AnalyserOptions parseAnalyserOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
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

BiquadFilterOptions parseBiquadFilterOptions(
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

OscillatorOptions parseOscillatorOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  OscillatorOptions options;

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

BaseAudioBufferSourceOptions parseBaseAudioBufferSourceOptions(
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
    options.pitchCorrection = static_cast<bool>(pitchCorrectionValue.getBool());
  }

  return options;
}

AudioBufferSourceOptions parseAudioBufferSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  AudioBufferSourceOptions options(parseBaseAudioBufferSourceOptions(runtime, optionsObject));

  if (optionsObject.hasProperty(runtime, "buffer")) {
    auto bufferHostObject = optionsObject.getProperty(runtime, "buffer")
                                .getObject(runtime)
                                .asHostObject<AudioBufferHostObject>(runtime);
    options.buffer = bufferHostObject->audioBuffer_;
  }

  auto loopValue = optionsObject.getProperty(runtime, "loop");
  if (loopValue.isBool()) {
    options.loop = static_cast<bool>(loopValue.getBool());
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

StreamerOptions parseStreamerOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  auto options = StreamerOptions();
  if (optionsObject.hasProperty(runtime, "streamPath")) {
    options.streamPath =
        optionsObject.getProperty(runtime, "streamPath").asString(runtime).utf8(runtime);
  }
  return options;
}

DelayOptions parseDelayOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
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

IIRFilterOptions parseIIRFilterOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
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

WaveShaperOptions parseWaveShaperOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
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

  if (optionsObject.hasProperty(runtime, "buffer")) {
    auto arrayBuffer = optionsObject.getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);

    options.curve = std::make_shared<AudioArrayBuffer>(
        reinterpret_cast<float *>(arrayBuffer.data(runtime)),
        static_cast<size_t>(arrayBuffer.size(runtime) / sizeof(float)));
  }
  return options;
}
} // namespace audioapi::option_parser
