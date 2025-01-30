#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "ContextState.h"
#include "OscillatorType.h"

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

class BaseAudioContext {
 public:
  BaseAudioContext();
  virtual ~BaseAudioContext() = default;

  std::string getState();
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] std::size_t getCurrentSampleFrame() const;
  std::shared_ptr<AudioDestinationNode> getDestination();

  std::shared_ptr<OscillatorNode> createOscillator();
  std::shared_ptr<GainNode> createGain();
  std::shared_ptr<StereoPannerNode> createStereoPanner();
  std::shared_ptr<BiquadFilterNode> createBiquadFilter();
  std::shared_ptr<AudioBufferSourceNode> createBufferSource();
  static std::shared_ptr<AudioBuffer>
  createBuffer(int numberOfChannels, int length, int sampleRate);
  std::shared_ptr<PeriodicWave> createPeriodicWave(
      float *real,
      float *imag,
      bool disableNormalization,
      int length);
  std::shared_ptr<AnalyserNode> createAnalyser();

  std::shared_ptr<AudioBuffer> decodeAudioDataSource(const std::string &path);
  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  AudioNodeManager *getNodeManager();
  [[nodiscard]] bool isRunning() const;
  [[nodiscard]] bool isClosed() const;

 protected:
  static std::string toString(ContextState state);
  std::shared_ptr<AudioDestinationNode> destination_;
  // init in AudioContext or OfflineContext constructor
  std::shared_ptr<AudioDecoder> audioDecoder_ {};

  // init in AudioContext or OfflineContext constructor
  int sampleRate_ {};
  ContextState state_ = ContextState::RUNNING;
  std::shared_ptr<AudioNodeManager> nodeManager_;

 private:
  std::shared_ptr<PeriodicWave> cachedSineWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSquareWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSawtoothWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedTriangleWave_ = nullptr;
};

} // namespace audioapi
