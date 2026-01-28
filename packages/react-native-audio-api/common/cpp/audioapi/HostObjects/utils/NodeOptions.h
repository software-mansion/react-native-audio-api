#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <audioapi/core/effects/PeriodicWave.h>
#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/types/BiquadFilterType.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/types/OverSampleType.h>
#include <audioapi/utils/AudioArray.h>

namespace audioapi {
struct AudioNodeOptions {
  int channelCount = 2;
  ChannelCountMode channelCountMode;
  ChannelInterpretation channelInterpretation;
};

struct GainOptions : AudioNodeOptions {
  float gain;
};

struct StereoPannerOptions : AudioNodeOptions {
  float pan;
};

struct ConvolverOptions : AudioNodeOptions {
  std::shared_ptr<AudioBuffer> bus;
  bool disableNormalization;
};

struct ConstantSourceOptions {
  float offset;
};

struct AnalyserOptions : AudioNodeOptions {
  int fftSize;
  float minDecibels;
  float maxDecibels;
  float smoothingTimeConstant;
};

struct BiquadFilterOptions : AudioNodeOptions {
  BiquadFilterType type;
  float frequency;
  float detune;
  float Q;
  float gain;
};

struct OscillatorOptions {
  std::shared_ptr<PeriodicWave> periodicWave;
  float frequency;
  float detune;
  OscillatorType type;
};

struct BaseAudioBufferSourceOptions {
  float detune;
  bool pitchCorrection;
  float playbackRate;
};

struct AudioBufferSourceOptions : BaseAudioBufferSourceOptions {
  std::shared_ptr<AudioBuffer> buffer;
  bool loop;
  float loopStart;
  float loopEnd;
};

struct StreamerOptions {
  std::string streamPath;
};

struct AudioBufferOptions {
  int numberOfChannels;
  size_t length;
  float sampleRate;
};

struct DelayOptions : AudioNodeOptions {
  float maxDelayTime;
  float delayTime;
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
  std::shared_ptr<AudioArray> curve;
  OverSampleType oversample;
};

} // namespace audioapi
