#pragma once

#include <audioapi/jsi/RuntimeLifecycleMonitor.h>
#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <audioapi/HostObjects/effects/PeriodicWaveHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/NodeOptions.h>

namespace audioapi::option_parser {
AudioNodeOptions parseAudioNodeOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  AudioNodeOptions options;

  options.channelCount =
      static_cast<int>(optionsObject.getProperty(runtime, "channelCount").getNumber());

  auto channelCountModeStr =
      optionsObject.getProperty(runtime, "channelCountMode").asString(runtime).utf8(runtime);

  if (channelCountModeStr == "max") {
    options.channelCountMode = ChannelCountMode::MAX;
  } else if (channelCountModeStr == "clamped-max") {
    options.channelCountMode = ChannelCountMode::CLAMPED_MAX;
  } else if (channelCountModeStr == "explicit") {
    options.channelCountMode = ChannelCountMode::EXPLICIT;
  }

  auto channelInterpretationStr =
      optionsObject.getProperty(runtime, "channelInterpretation").asString(runtime).utf8(runtime);

  if (channelInterpretationStr == "speakers") {
    options.channelInterpretation = ChannelInterpretation::SPEAKERS;
  } else if (channelInterpretationStr == "discrete") {
    options.channelInterpretation = ChannelInterpretation::DISCRETE;
  }

  return options;
}

GainOptions parseGainOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  GainOptions options(parseAudioNodeOptions(runtime, optionsObject));
  options.gain = static_cast<float>(optionsObject.getProperty(runtime, "gain").getNumber());
  return options;
}

StereoPannerOptions parseStereoPannerOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  StereoPannerOptions options(parseAudioNodeOptions(runtime, optionsObject));
  options.pan = static_cast<float>(optionsObject.getProperty(runtime, "pan").getNumber());
  return options;
}

ConvolverOptions parseConvolverOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  ConvolverOptions options(parseAudioNodeOptions(runtime, optionsObject));
  options.disableNormalization =
      optionsObject.getProperty(runtime, "disableNormalization").getBool();
  if (optionsObject.hasProperty(runtime, "buffer")) {
    auto bufferHostObject = optionsObject.getProperty(runtime, "buffer")
                                .getObject(runtime)
                                .asHostObject<AudioBufferHostObject>(runtime);
    options.bus = bufferHostObject->audioBuffer_;
  }
  return options;
}

ConstantSourceOptions parseConstantSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  ConstantSourceOptions options;
  options.offset = static_cast<float>(optionsObject.getProperty(runtime, "offset").getNumber());
  return options;
}

AnalyserOptions parseAnalyserOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  AnalyserOptions options(parseAudioNodeOptions(runtime, optionsObject));
  options.fftSize = static_cast<int>(optionsObject.getProperty(runtime, "fftSize").getNumber());
  options.minDecibels =
      static_cast<float>(optionsObject.getProperty(runtime, "minDecibels").getNumber());
  options.maxDecibels =
      static_cast<float>(optionsObject.getProperty(runtime, "maxDecibels").getNumber());
  options.smoothingTimeConstant =
      static_cast<float>(optionsObject.getProperty(runtime, "smoothingTimeConstant").getNumber());
  return options;
}

BiquadFilterOptions parseBiquadFilterOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  BiquadFilterOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto typeStr = optionsObject.getProperty(runtime, "type").asString(runtime).utf8(runtime);

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

  options.frequency =
      static_cast<float>(optionsObject.getProperty(runtime, "frequency").getNumber());
  options.detune = static_cast<float>(optionsObject.getProperty(runtime, "detune").getNumber());
  options.Q = static_cast<float>(optionsObject.getProperty(runtime, "Q").getNumber());
  options.gain = static_cast<float>(optionsObject.getProperty(runtime, "gain").getNumber());

  return options;
}

OscillatorOptions parseOscillatorOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  OscillatorOptions options;

  auto typeStr = optionsObject.getProperty(runtime, "type").asString(runtime).utf8(runtime);

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

  options.frequency =
      static_cast<float>(optionsObject.getProperty(runtime, "frequency").getNumber());
  options.detune = static_cast<float>(optionsObject.getProperty(runtime, "detune").getNumber());

  if (optionsObject.hasProperty(runtime, "periodicWave")) {
    auto periodicWaveHostObject = optionsObject.getProperty(runtime, "periodicWave")
                                      .getObject(runtime)
                                      .asHostObject<PeriodicWaveHostObject>(runtime);
    options.periodicWave = periodicWaveHostObject->periodicWave_;
  }

  return options;
}

BaseAudioBufferSourceOptions parseBaseAudioBufferSourceOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  BaseAudioBufferSourceOptions options;
  options.detune = static_cast<float>(optionsObject.getProperty(runtime, "detune").getNumber());
  options.playbackRate =
      static_cast<float>(optionsObject.getProperty(runtime, "playbackRate").getNumber());
  options.pitchCorrection =
      static_cast<bool>(optionsObject.getProperty(runtime, "pitchCorrection").getBool());
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
  options.loop = static_cast<bool>(optionsObject.getProperty(runtime, "loop").getBool());
  options.loopStart =
      static_cast<float>(optionsObject.getProperty(runtime, "loopStart").getNumber());
  options.loopEnd = static_cast<float>(optionsObject.getProperty(runtime, "loopEnd").getNumber());
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

AudioBufferOptions parseAudioBufferOptions(
    jsi::Runtime &runtime,
    const jsi::Object &optionsObject) {
  AudioBufferOptions options;
  options.numberOfChannels =
      static_cast<int>(optionsObject.getProperty(runtime, "numberOfChannels").getNumber());
  options.length = static_cast<size_t>(optionsObject.getProperty(runtime, "length").getNumber());
  options.sampleRate =
      static_cast<float>(optionsObject.getProperty(runtime, "sampleRate").getNumber());
  return options;
}

DelayOptions parseDelayOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  DelayOptions options(parseAudioNodeOptions(runtime, optionsObject));
  options.maxDelayTime =
      static_cast<float>(optionsObject.getProperty(runtime, "maxDelayTime").getNumber());
  options.delayTime =
      static_cast<float>(optionsObject.getProperty(runtime, "delayTime").getNumber());
  return options;
}

IIRFilterOptions parseIIRFilterOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  IIRFilterOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto feedforwardArray =
      optionsObject.getProperty(runtime, "feedforward").asObject(runtime).asArray(runtime);
  size_t feedforwardLength = feedforwardArray.size(runtime);
  options.feedforward.reserve(feedforwardLength);
  for (size_t i = 0; i < feedforwardLength; ++i) {
    options.feedforward.push_back(
        static_cast<float>(feedforwardArray.getValueAtIndex(runtime, i).getNumber()));
  }

  auto feedbackArray =
      optionsObject.getProperty(runtime, "feedback").asObject(runtime).asArray(runtime);
  size_t feedbackLength = feedbackArray.size(runtime);
  options.feedback.reserve(feedbackLength);
  for (size_t i = 0; i < feedbackLength; ++i) {
    options.feedback.push_back(
        static_cast<float>(feedbackArray.getValueAtIndex(runtime, i).getNumber()));
  }

  return options;
}

WaveShaperOptions parseWaveShaperOptions(jsi::Runtime &runtime, const jsi::Object &optionsObject) {
  WaveShaperOptions options(parseAudioNodeOptions(runtime, optionsObject));

  auto oversampleStr =
      optionsObject.getProperty(runtime, "oversample").asString(runtime).utf8(runtime);

  if (oversampleStr == "none") {
    options.oversample = OverSampleType::OVERSAMPLE_NONE;
  } else if (oversampleStr == "2x") {
    options.oversample = OverSampleType::OVERSAMPLE_2X;
  } else if (oversampleStr == "4x") {
    options.oversample = OverSampleType::OVERSAMPLE_4X;
  }

  if (optionsObject.hasProperty(runtime, "buffer")) {
    auto arrayBuffer = optionsObject.getPropertyAsObject(runtime, "buffer").getArrayBuffer(runtime);

    options.curve = std::make_shared<AudioArray>(
        reinterpret_cast<float *>(arrayBuffer.data(runtime)),
        static_cast<size_t>(arrayBuffer.size(runtime) / sizeof(float)));
  }
  return options;
}
} // namespace audioapi::option_parser
