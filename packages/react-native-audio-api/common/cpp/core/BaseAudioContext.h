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

#ifdef ANDROID
class AudioPlayer;
#else
class IOSAudioPlayer;
#endif

class BaseAudioContext {
 public:
  BaseAudioContext();
  ~BaseAudioContext();

  std::string getState();
  [[nodiscard]] int getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] int getBufferSizeInFrames() const;
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

  std::shared_ptr<AudioBuffer> decodeAudioDataSource(const std::string &path);
  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  std::function<void(AudioBus *, int)> renderAudio();
  AudioNodeManager *getNodeManager();
  [[nodiscard]] bool isRunning() const;
  [[nodiscard]] bool isClosed() const;

 protected:
  static std::string toString(ContextState state);
  std::shared_ptr<AudioDestinationNode> destination_;
  std::shared_ptr<AudioDecoder> audioDecoder_;

#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif

  int sampleRate_;
  int bufferSizeInFrames_;
  ContextState state_ = ContextState::RUNNING;
  std::shared_ptr<AudioNodeManager> nodeManager_;

 private:
  std::shared_ptr<PeriodicWave> cachedSineWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSquareWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSawtoothWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedTriangleWave_ = nullptr;
};

} // namespace audioapi
