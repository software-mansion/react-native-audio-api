#pragma once

#include <audioapi/core/types/ContextState.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/Graph.hpp>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CrossThreadEventScheduler.hpp>

#include <atomic>
#include <cassert>
#include <complex>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

class GainNode;
class DelayNode;
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

  std::shared_ptr<PeriodicWave> createPeriodicWave(
      const std::vector<std::complex<float>> &complexData,
      bool disableNormalization,
      int length) const;

  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  AudioGraphManager *getGraphManager() const;
  std::shared_ptr<utils::graph::Graph> getGraph() const;
  std::shared_ptr<IAudioEventHandlerRegistry> getAudioEventHandlerRegistry() const;
  const RuntimeRegistry &getRuntimeRegistry() const;
  utils::DisposerImpl<utils::graph::Graph::kDisposerPayloadSize> *getDisposer() const;

  virtual void initialize();

  void inline processAudioEvents() {
    audioEventScheduler_.processAllEvents(*this);
  }

  template <typename F>
  bool inline scheduleAudioEvent(F &&event) noexcept {
    if (getState() != ContextState::RUNNING) {
      processAudioEvents();
      event(*this);
      return true;
    }

    return audioEventScheduler_.scheduleEvent(std::forward<F>(event));
  }

 protected:
  std::shared_ptr<AudioDestinationNode> destination_;

 private:
  std::atomic<ContextState> state_;
  std::atomic<float> sampleRate_;
  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  RuntimeRegistry runtimeRegistry_;

  std::shared_ptr<PeriodicWave> cachedSineWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSquareWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedSawtoothWave_ = nullptr;
  std::shared_ptr<PeriodicWave> cachedTriangleWave_ = nullptr;

  static constexpr size_t AUDIO_SCHEDULER_CAPACITY = 1024;
  CrossThreadEventScheduler<BaseAudioContext> audioEventScheduler_;
  std::unique_ptr<AudioGraphManager> graphManager_;

  std::unique_ptr<utils::DisposerImpl<utils::graph::Graph::kDisposerPayloadSize>> disposer_;
  std::shared_ptr<utils::graph::Graph> graph_;

  [[nodiscard]] virtual bool isDriverRunning() const = 0;
};

} // namespace audioapi
