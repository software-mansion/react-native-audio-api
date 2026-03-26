#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <audioapi/core/effects/PeriodicWave.h>
#include <audioapi/core/types/BiquadFilterType.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/utils/AudioArray.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

namespace audioapi {
struct AudioNodeOptions {
  int channelCount = 2;
  ChannelCountMode channelCountMode = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation = ChannelInterpretation::SPEAKERS;
  int numberOfInputs = 1;
  int numberOfOutputs = 1;
  bool requiresTailProcessing = false;
};

struct AudioDestinationOptions : AudioNodeOptions {
  AudioDestinationOptions() {
    numberOfOutputs = 0;
    channelCountMode = ChannelCountMode::EXPLICIT;
  }
};

struct AudioScheduledSourceNodeOptions : AudioNodeOptions {
  AudioScheduledSourceNodeOptions() {
    numberOfInputs = 0;
  }
};

struct GainOptions : AudioNodeOptions {
  float gain = 1.0f;
};

struct StereoPannerOptions : AudioNodeOptions {
  float pan = 0.0f;

  StereoPannerOptions() {
    channelCountMode = ChannelCountMode::CLAMPED_MAX;
  }

  explicit StereoPannerOptions(AudioNodeOptions options) : AudioNodeOptions(options) {
    channelCountMode = ChannelCountMode::CLAMPED_MAX;
  }
};

struct ConvolverOptions : AudioNodeOptions {
  bool disableNormalization = false;
  std::shared_ptr<AudioBuffer> buffer = nullptr;

  ConvolverOptions() {
    requiresTailProcessing = true;
  }

  explicit ConvolverOptions(AudioNodeOptions options) : AudioNodeOptions(options) {
    requiresTailProcessing = true;
  }
};

struct ConstantSourceOptions : AudioScheduledSourceNodeOptions {
  float offset = 1.0f;
};

struct AnalyserOptions : AudioNodeOptions {
  static constexpr int kDefaultFftSize = 2048;
  static constexpr float kDefaultMinDecibels = -100.0f;
  static constexpr float kDefaultMaxDecibels = -30.0f;
  static constexpr float kDefaultSmoothingTimeConstant = 0.8f;
  int fftSize = kDefaultFftSize;
  float minDecibels = kDefaultMinDecibels;
  float maxDecibels = kDefaultMaxDecibels;
  float smoothingTimeConstant = kDefaultSmoothingTimeConstant;
};

struct BiquadFilterOptions : AudioNodeOptions {
  static constexpr float kDefaultFrequency = 350.0f;
  BiquadFilterType type = BiquadFilterType::LOWPASS;
  float frequency = kDefaultFrequency;
  float detune = 0.0f;
  float Q = 1.0f;
  float gain = 0.0f;
};

struct OscillatorOptions : AudioScheduledSourceNodeOptions {
  static constexpr float kDefaultFrequency = 440.0f;
  std::shared_ptr<PeriodicWave> periodicWave = nullptr;
  float frequency = kDefaultFrequency;
  float detune = 0.0f;
  OscillatorType type = OscillatorType::SINE;
};

struct BaseAudioBufferSourceOptions : AudioScheduledSourceNodeOptions {
  bool pitchCorrection = false;
  float detune = 0.0f;
  float playbackRate = 1.0f;
  int onPositionChangedInterval = 100;
};

struct AudioBufferSourceOptions : BaseAudioBufferSourceOptions {
  std::shared_ptr<AudioBuffer> buffer = nullptr;
  float loopStart = 0.0f;
  float loopEnd = 0.0f;
  bool loop = false;
  bool loopSkip = false;

  explicit AudioBufferSourceOptions(BaseAudioBufferSourceOptions options)
      : BaseAudioBufferSourceOptions(options) {
    channelCount = 1;
  }
};

struct StreamerOptions : AudioScheduledSourceNodeOptions {
  std::string streamPath;
};

struct DelayOptions : AudioNodeOptions {
  float maxDelayTime = 1.0f;
  float delayTime = 0.0f;

  DelayOptions() {
    requiresTailProcessing = true;
  }

  explicit DelayOptions(AudioNodeOptions options) : AudioNodeOptions(options) {
    requiresTailProcessing = true;
  }
};

struct IIRFilterOptions : AudioNodeOptions {
  std::vector<float> feedforward;
  std::vector<float> feedback;

  IIRFilterOptions() = default;

  explicit IIRFilterOptions(AudioNodeOptions options) : AudioNodeOptions(options) {}

  IIRFilterOptions(const std::vector<float> &ff, const std::vector<float> &fb)
      : feedforward(ff), feedback(fb) {}

  IIRFilterOptions(std::vector<float> &&ff, std::vector<float> &&fb)
      : feedforward(std::move(ff)), feedback(std::move(fb)) {}
};

struct WaveShaperOptions : AudioNodeOptions {
  std::shared_ptr<AudioArray> curve{nullptr};
  OverSampleType oversample = OverSampleType::OVERSAMPLE_NONE;

  WaveShaperOptions() {
    // to change after graph processing improvement - should be max
    channelCountMode = ChannelCountMode::CLAMPED_MAX;
  }

  explicit WaveShaperOptions(const AudioNodeOptions &options) : AudioNodeOptions(options) {
    // to change after graph processing improvement - should be max
    channelCountMode = ChannelCountMode::CLAMPED_MAX;
  }
};

} // namespace audioapi
