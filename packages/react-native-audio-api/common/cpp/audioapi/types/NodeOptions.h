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
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>
#include <audioapi/utils/AudioBuffer.h>

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

  explicit StereoPannerOptions(AudioNodeOptions &&options) : AudioNodeOptions(options) {
    channelCountMode = ChannelCountMode::CLAMPED_MAX;
  }
};

struct ConvolverOptions : AudioNodeOptions {
  bool disableNormalization = false;
  std::shared_ptr<AudioBuffer> buffer;

  ConvolverOptions() {
    requiresTailProcessing = true;
  }

  explicit ConvolverOptions(AudioNodeOptions &&options) : AudioNodeOptions(options) {
    requiresTailProcessing = true;
  }
};

struct ConstantSourceOptions : AudioScheduledSourceNodeOptions {
  float offset = 1.0f;
};

struct AnalyserOptions : AudioNodeOptions {
  int fftSize = 2048;
  float minDecibels = -100.0f;
  float maxDecibels = -30.0f;
  float smoothingTimeConstant = 0.8f;
};

struct BiquadFilterOptions : AudioNodeOptions {
  BiquadFilterType type = BiquadFilterType::LOWPASS;
  float frequency = 350.0f;
  float detune = 0.0f;
  float Q = 1.0f;
  float gain = 0.0f;
};

struct OscillatorOptions : AudioScheduledSourceNodeOptions {
  std::shared_ptr<PeriodicWave> periodicWave;
  float frequency = 440.0f;
  float detune = 0.0f;
  OscillatorType type = OscillatorType::SINE;
};

struct BaseAudioBufferSourceOptions : AudioScheduledSourceNodeOptions {
  bool pitchCorrection = false;
  float detune = 0.0f;
  float playbackRate = 1.0f;
};

struct AudioBufferSourceOptions : BaseAudioBufferSourceOptions {
  std::shared_ptr<AudioBuffer> buffer;
  float loopStart = 0.0f;
  float loopEnd = 0.0f;
  bool loop = false;
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

  explicit DelayOptions(AudioNodeOptions &&options) : AudioNodeOptions(options) {
    requiresTailProcessing = true;
  }
};

struct IIRFilterOptions : AudioNodeOptions {
  std::vector<float> feedforward;
  std::vector<float> feedback;

  IIRFilterOptions() = default;

  explicit IIRFilterOptions(const AudioNodeOptions options) : AudioNodeOptions(options) {}

  IIRFilterOptions(const std::vector<float> &ff, const std::vector<float> &fb)
      : feedforward(ff), feedback(fb) {}

  IIRFilterOptions(std::vector<float> &&ff, std::vector<float> &&fb)
      : feedforward(std::move(ff)), feedback(std::move(fb)) {}
};

struct WaveShaperOptions : AudioNodeOptions {
  std::shared_ptr<AudioArrayBuffer> curve;
  OverSampleType oversample = OverSampleType::OVERSAMPLE_NONE;

  WaveShaperOptions() {
    // to change after graph processing improvement - should be max
    channelCountMode = ChannelCountMode::CLAMPED_MAX;
  }

  explicit WaveShaperOptions(const AudioNodeOptions &&options) : AudioNodeOptions(options) {
    // to change after graph processing improvement - should be max
    channelCountMode = ChannelCountMode::CLAMPED_MAX;
  }
};

} // namespace audioapi
