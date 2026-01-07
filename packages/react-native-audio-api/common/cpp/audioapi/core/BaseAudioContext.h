#pragma once

#include <audioapi/core/types/ContextState.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <cassert>
#include <complex>
#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace audioapi {

class AudioBus;
class GainNode;
class DelayNode;
class AudioBuffer;
class PeriodicWave;
class OscillatorNode;
class ConstantSourceNode;
class StereoPannerNode;
class AudioNodeManager;
class BiquadFilterNode;
class IIRFilterNode;
class AudioDestinationNode;
class AudioBufferSourceNode;
class AudioBufferQueueSourceNode;
class AnalyserNode;
class AudioEventHandlerRegistry;
class ConvolverNode;
class IAudioEventHandlerRegistry;
class RecorderAdapterNode;
class WorkletSourceNode;
class WorkletNode;
class WorkletProcessingNode;
class StreamerNode;
class WaveShaperNode;

class BaseAudioContext : public std::enable_shared_from_this<BaseAudioContext> {
 public:
  explicit BaseAudioContext(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const RuntimeRegistry &runtimeRegistry);
  virtual ~BaseAudioContext() = default;

  virtual void initialize();

  std::string getState();
  [[nodiscard]] float getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] std::size_t getCurrentSampleFrame() const;
  std::shared_ptr<AudioDestinationNode> getDestination();

  std::shared_ptr<RecorderAdapterNode> createRecorderAdapter();
  std::shared_ptr<WorkletSourceNode> createWorkletSourceNode(
      std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
      std::weak_ptr<worklets::WorkletRuntime> runtime,
      bool shouldLockRuntime = true);
  std::shared_ptr<WorkletNode> createWorkletNode(
      std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
      std::weak_ptr<worklets::WorkletRuntime> runtime,
      size_t bufferLength,
      size_t inputChannelCount,
      bool shouldLockRuntime = true);
  std::shared_ptr<WorkletProcessingNode> createWorkletProcessingNode(
      std::shared_ptr<worklets::SerializableWorklet> &shareableWorklet,
      std::weak_ptr<worklets::WorkletRuntime> runtime,
      bool shouldLockRuntime = true);
  std::shared_ptr<OscillatorNode> createOscillator();
  std::shared_ptr<ConstantSourceNode> createConstantSource();
  std::shared_ptr<StreamerNode> createStreamer();
  std::shared_ptr<GainNode> createGain();
  std::shared_ptr<DelayNode> createDelay(float maxDelayTime);
  std::shared_ptr<StereoPannerNode> createStereoPanner();
  std::shared_ptr<BiquadFilterNode> createBiquadFilter();
  std::shared_ptr<IIRFilterNode> createIIRFilter(
      const std::vector<float> &feedforward,
      const std::vector<float> &feedback);
  std::shared_ptr<AudioBufferSourceNode> createBufferSource(bool pitchCorrection);
  std::shared_ptr<AudioBufferQueueSourceNode> createBufferQueueSource(bool pitchCorrection);
  static std::shared_ptr<AudioBuffer>
  createBuffer(int numberOfChannels, size_t length, float sampleRate);
  std::shared_ptr<PeriodicWave> createPeriodicWave(
      const std::vector<std::complex<float>> &complexData,
      bool disableNormalization,
      int length);
  std::shared_ptr<AnalyserNode> createAnalyser();
  std::shared_ptr<ConvolverNode> createConvolver(
      std::shared_ptr<AudioBuffer> buffer,
      bool disableNormalization);
  std::shared_ptr<WaveShaperNode> createWaveShaper();

  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  [[nodiscard]] float getNyquistFrequency() const;
  AudioNodeManager *getNodeManager();

  [[nodiscard]] bool isRunning() const;
  [[nodiscard]] bool isSuspended() const;
  [[nodiscard]] bool isClosed() const;

 protected:
  static std::string toString(ContextState state);

  std::shared_ptr<AudioDestinationNode> destination_;
  // init in AudioContext or OfflineContext constructor
  float sampleRate_{};
  ContextState state_ = ContextState::RUNNING;
  std::shared_ptr<AudioNodeManager> nodeManager_;

 private:
  std::shared_ptr<PeriodicWave> cachedSineWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSquareWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSawtoothWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedTriangleWave_ = nullptr;

  [[nodiscard]] virtual bool isDriverRunning() const = 0;

 public:
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  RuntimeRegistry runtimeRegistry_;
};

} // namespace audioapi
