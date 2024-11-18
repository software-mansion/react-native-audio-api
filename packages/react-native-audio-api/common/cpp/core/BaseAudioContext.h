#pragma once

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

class AudioDestinationNode;
class OscillatorNode;
class GainNode;
class StereoPannerNode;
class BiquadFilterNode;
class AudioBufferSourceNode;
class AudioBuffer;

#ifdef ANDROID
class AudioPlayer;
#else
class IOSAudioPlayer;
#endif

class BaseAudioContext {
 public:
  BaseAudioContext();
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
  std::function<void(AudioBus*, int)> renderAudio();

 protected:
  enum class State { SUSPENDED, RUNNING, CLOSED };
  static std::string toString(State state);
  std::shared_ptr<AudioDestinationNode> destination_;

#ifdef ANDROID
  std::shared_ptr<AudioPlayer> audioPlayer_;
#else
  std::shared_ptr<IOSAudioPlayer> audioPlayer_;
#endif

  State state_ = State::RUNNING;
  int sampleRate_;
  int bufferSizeInFrames_;
};

} // namespace audioapi
