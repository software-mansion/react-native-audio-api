#pragma once

#include <audioapi/core/types/ContextState.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>

#include <atomic>
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
class AudioGraphManager;
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
class WaveShaperNode;
class WorkletSourceNode;
class WorkletNode;
class WorkletProcessingNode;
class StreamerNode;
struct GainOptions;
struct StereoPannerOptions;
struct ConvolverOptions;
struct ConstantSourceOptions;
struct AnalyserOptions;
struct BiquadFilterOptions;
struct OscillatorOptions;
struct BaseAudioBufferSourceOptions;
struct AudioBufferSourceOptions;
struct StreamerOptions;
struct DelayOptions;
struct IIRFilterOptions;
struct WaveShaperOptions;

class BaseAudioContext : public std::enable_shared_from_this<BaseAudioContext> {
 public:
  explicit BaseAudioContext(
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const RuntimeRegistry &runtimeRegistry);
  virtual ~BaseAudioContext() = default;

  ContextState getState();
  [[nodiscard]] float getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] std::size_t getCurrentSampleFrame() const;
  std::shared_ptr<AudioDestinationNode> getDestination() const;

  void setState(ContextState state);

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
  std::shared_ptr<DelayNode> createDelay(const DelayOptions &options);
  std::shared_ptr<IIRFilterNode> createIIRFilter(const IIRFilterOptions &options);
  std::shared_ptr<OscillatorNode> createOscillator(const OscillatorOptions &options);
  std::shared_ptr<ConstantSourceNode> createConstantSource(const ConstantSourceOptions &options);
  std::shared_ptr<StreamerNode> createStreamer(const StreamerOptions &options);
  std::shared_ptr<GainNode> createGain(const GainOptions &options);
  std::shared_ptr<StereoPannerNode> createStereoPanner(const StereoPannerOptions &options);
  std::shared_ptr<BiquadFilterNode> createBiquadFilter(const BiquadFilterOptions &options);
  std::shared_ptr<AudioBufferSourceNode> createBufferSource(
      const AudioBufferSourceOptions &options);
  std::shared_ptr<AudioBufferQueueSourceNode> createBufferQueueSource(
      const BaseAudioBufferSourceOptions &options);
  std::shared_ptr<PeriodicWave> createPeriodicWave(
      const std::vector<std::complex<float>> &complexData,
      bool disableNormalization,
      int length) const;
  std::shared_ptr<AnalyserNode> createAnalyser(const AnalyserOptions &options);
  std::shared_ptr<ConvolverNode> createConvolver(const ConvolverOptions &options);
  std::shared_ptr<WaveShaperNode> createWaveShaper(const WaveShaperOptions &options);

  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  [[nodiscard]] float getNyquistFrequency() const;
  std::shared_ptr<AudioGraphManager> getGraphManager() const;
  std::shared_ptr<IAudioEventHandlerRegistry> getAudioEventHandlerRegistry() const;
  const RuntimeRegistry &getRuntimeRegistry() const;

  virtual void initialize();

 protected:
  std::shared_ptr<AudioDestinationNode> destination_;

 private:
  std::atomic<ContextState> state_;
  std::atomic<float> sampleRate_;
  std::shared_ptr<AudioGraphManager> graphManager_;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  RuntimeRegistry runtimeRegistry_;

  std::shared_ptr<PeriodicWave> cachedSineWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSquareWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSawtoothWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedTriangleWave_ = nullptr;

  [[nodiscard]] virtual bool isDriverRunning() const = 0;
};

} // namespace audioapi
