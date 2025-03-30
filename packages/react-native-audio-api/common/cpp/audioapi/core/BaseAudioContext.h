#pragma once

#include <audioapi/core/types/ContextState.h>
#include <audioapi/core/types/OscillatorType.h>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <cstddef>
#include <cassert>

namespace audioapi {

class AudioBus;
class GainNode;
class AudioBuffer;
class PeriodicWave;
class OscillatorNode;
class StereoPannerNode;
class AudioNodeManager;
class BiquadFilterNode;
class AudioDestinationNode;
class AudioBufferSourceNode;
class AudioDecoder;
class AnalyserNode;
class StretcherNode;

class BaseAudioContext {
 public:
  BaseAudioContext();
  virtual ~BaseAudioContext() = default;

  std::string getState();
  [[nodiscard]] float getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] std::size_t getCurrentSampleFrame() const;
  std::shared_ptr<AudioDestinationNode> getDestination();

  std::shared_ptr<OscillatorNode> createOscillator();
  std::shared_ptr<GainNode> createGain();
  std::shared_ptr<StereoPannerNode> createStereoPanner();
  std::shared_ptr<BiquadFilterNode> createBiquadFilter();
  std::shared_ptr<AudioBufferSourceNode> createBufferSource();
  static std::shared_ptr<AudioBuffer>
  createBuffer(int numberOfChannels, size_t length, float sampleRate);
  std::shared_ptr<PeriodicWave> createPeriodicWave(
      float *real,
      float *imag,
      bool disableNormalization,
      int length);
  std::shared_ptr<AnalyserNode> createAnalyser();
  std::shared_ptr<StretcherNode> createStretcher();

  std::shared_ptr<AudioBuffer> decodeAudioDataSource(const std::string &path);

  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  [[nodiscard]] float getNyquistFrequency() const;
  AudioNodeManager *getNodeManager();

  [[nodiscard]] bool isRunning() const;
  [[nodiscard]] bool isClosed() const;

  struct Stats {
    unsigned numAudioBuffers;
    unsigned numAudioBufferSources;
    unsigned numOscillators;
    unsigned numAudioArrays;
    unsigned numAudioBuses;
    unsigned numAudioManagers;
    unsigned numBiquads;
    unsigned numGains;
    unsigned numPeriodicWaves;
    unsigned numStereoPanners;
    unsigned numStretchers;
    unsigned numDestinations;
    unsigned numAnalysers;
    unsigned numAudioParams;

    Stats(): numAudioBuffers(0), numAudioBufferSources(0),
            numOscillators(0), numAudioArrays(0), numAudioBuses(0),
            numAudioManagers(0), numBiquads(0), numGains(0),
            numPeriodicWaves(0), numStereoPanners(0), numStretchers(0),
            numDestinations(0), numAnalysers(0), numAudioParams(0) {}
  };

  Stats stats;

 protected:
  friend class AudioNode;
  static std::string toString(ContextState state);

  std::shared_ptr<AudioDestinationNode> destination_;
  // init in AudioContext or OfflineContext constructor
  std::shared_ptr<AudioDecoder> audioDecoder_ {};
  // init in AudioContext or OfflineContext constructor
  float sampleRate_ {};
  ContextState state_ = ContextState::RUNNING;
  std::shared_ptr<AudioNodeManager> nodeManager_;

 private:
  std::shared_ptr<PeriodicWave> cachedSineWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSquareWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSawtoothWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedTriangleWave_ = nullptr;
};

} // namespace audioapi
