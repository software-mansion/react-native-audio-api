#pragma once

#include <audioapi/core/types/ContextState.h>
#include <audioapi/core/types/OscillatorType.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/Disposer.hpp>
#include <audioapi/core/utils/graph/Graph.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/CrossThreadEventScheduler.hpp>

#include <audioapi/utils/Macros.h>
#include <atomic>
#include <cassert>
#include <complex>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

class AudioEventHandlerRegistry;
class IAudioEventHandlerRegistry;
class PeriodicWave;
class AudioDestinationNode;

class BaseAudioContext : public std::enable_shared_from_this<BaseAudioContext> {
 public:
  explicit BaseAudioContext(
      float sampleRate,
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      const RuntimeRegistry &runtimeRegistry);
  virtual ~BaseAudioContext() = default;
  DELETE_COPY_AND_MOVE(BaseAudioContext);

  ContextState getState();
  [[nodiscard]] float getSampleRate() const;
  [[nodiscard]] double getCurrentTime() const;
  [[nodiscard]] std::size_t getCurrentSampleFrame() const;

  void setState(ContextState state);

  std::shared_ptr<PeriodicWave> createPeriodicWave(
      const std::vector<std::complex<float>> &complexData,
      bool disableNormalization,
      int length) const;

  std::shared_ptr<PeriodicWave> getBasicWaveForm(OscillatorType type);
  std::shared_ptr<utils::graph::Graph> getGraph() const;
  std::shared_ptr<IAudioEventHandlerRegistry> getAudioEventHandlerRegistry() const;
  const RuntimeRegistry &getRuntimeRegistry() const;
  utils::DisposerImpl<DISPOSER_PAYLOAD_SIZE> *getDisposer() const;

  /// @brief Assigns the audio destination node to the context.
  /// @param destination The audio destination node to be associated with the context.
  /// @note This method must be called before the audio context can be used for processing audio.
  virtual void initialize(const AudioDestinationNode *destination);

  void processAudioEvents() {
    // Drain JS-thread scheduler first, then the GC-thread scheduler.
    // This preserves causality for the common case where a JS-thread
    // produced event logically happens-before a GC-thread cleanup event
    // on the same node (e.g. `onEnded = id` followed by HostObject
    // finalization that clears the callback id).
    audioEventScheduler_.processAllEvents(*this);
    gcAudioEventScheduler_.processAllEvents(*this);
  }

  template <typename F>
  bool scheduleAudioEvent(F &&event) noexcept {
    if (getState() != ContextState::RUNNING) {
      processAudioEvents();
      event(*this);
      return true;
    }

    return audioEventScheduler_.scheduleEvent(std::forward<F>(event));
  }

  /// @brief Schedule an audio event produced on the JS runtime's finalizer
  /// (GC) thread. Uses a dedicated SPSC channel so the main
  /// `audioEventScheduler_` stays single-producer (JS thread).
  ///
  /// Call this from JSI HostObject destructors (which on Hermes fire on the
  /// concurrent GC thread) instead of `scheduleAudioEvent`, otherwise the
  /// GC thread and the JS thread will race on the same SPSC channel.
  ///
  /// @note Unlike `scheduleAudioEvent`, this does NOT fall through to a
  /// synchronous `event(*this)` when the context is not RUNNING. The
  /// finalizer thread must never touch context state directly, and the
  /// event handler itself must be safe to skip if the audio thread never
  /// drains it (e.g. context already closed).
  template <typename F>
  bool scheduleGCEvent(F &&event) noexcept {
    return gcAudioEventScheduler_.scheduleEvent(std::forward<F>(event));
  }

  void processGraph(DSPAudioBuffer *buffer, int numFrames);

 protected:
  std::atomic<std::size_t> currentSampleFrame_{0};
  const AudioDestinationNode *destination_;

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
  static constexpr size_t GC_AUDIO_SCHEDULER_CAPACITY = 256;
  CrossThreadEventScheduler<BaseAudioContext> audioEventScheduler_;
  /// Dedicated single-producer SPSC scheduler for events produced on the
  /// JS runtime's finalizer (GC) thread. Keeps `audioEventScheduler_`
  /// single-producer (JS thread) and avoids the MPSC-on-SPSC data race.
  CrossThreadEventScheduler<BaseAudioContext> gcAudioEventScheduler_;

  std::unique_ptr<utils::DisposerImpl<DISPOSER_PAYLOAD_SIZE>> disposer_;
  std::shared_ptr<utils::graph::Graph> graph_;

  [[nodiscard]] virtual bool isDriverRunning() const = 0;
};

} // namespace audioapi
