#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <memory>

namespace audioapi {

/// Properties every platform player needs.
class CommonPlayerProperties {
 protected:
  explicit CommonPlayerProperties(std::atomic<uint32_t> &currentRenders)
      : currentRenders_(currentRenders) {}

  std::function<void(DSPAudioBuffer *, int)> renderAudio_;
  float sampleRate_{0};
  int channelCount_{0};
  std::reference_wrapper<std::atomic<uint32_t>> currentRenders_;
};

class CommonPlayer : protected CommonPlayerProperties {
 public:
  DELETE_COPY_AND_MOVE(CommonPlayer);
  virtual ~CommonPlayer() = default;

  virtual bool start() = 0;
  virtual void stop() = 0;
  virtual bool resume() = 0;
  virtual void suspend() = 0;
  virtual void cleanup() = 0;

  [[nodiscard]] virtual double getBaseLatency() const = 0;
  [[nodiscard]] virtual double getOutputLatency() const = 0;

  [[nodiscard]] virtual bool isRunning() const = 0;

 protected:
  explicit CommonPlayer(const CommonPlayerProperties &properties)
      : CommonPlayerProperties(properties),
        buffer_(std::make_shared<DSPAudioBuffer>(RENDER_QUANTUM_SIZE, channelCount_, sampleRate_)) {
  }

  std::shared_ptr<DSPAudioBuffer> buffer_;
  /// Whether the player was last started or resumed rather than stopped or suspended.
  /// A platform's `isRunning()` may also check its own driver state.
  std::atomic<bool> isRunning_{false};
};

} // namespace audioapi
