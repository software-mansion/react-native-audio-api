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
    audioEventScheduler_.processAllEvents(*this);
  }

  template <typename F>
  bool scheduleAudioEvent(F &&event) noexcept { // NOLINT(cppcoreguidelines-missing-std-forward)
    if (getState() != ContextState::RUNNING) {
      processAudioEvents();
      event(*this);
      return true;
    }

    return audioEventScheduler_.scheduleEvent(std::forward<F>(event));
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
  CrossThreadEventScheduler<BaseAudioContext> audioEventScheduler_;

  std::unique_ptr<utils::DisposerImpl<DISPOSER_PAYLOAD_SIZE>> disposer_;
  std::shared_ptr<utils::graph::Graph> graph_;

  [[nodiscard]] virtual bool isDriverRunning() const = 0;
};

} // namespace audioapi
