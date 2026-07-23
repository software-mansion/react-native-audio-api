#pragma once

#include <audioapi/core/CommonPlayer.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace audioapi {

class NodeAudioPlayer final : public CommonPlayer {
 public:
  NodeAudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      int channelCount);
  ~NodeAudioPlayer() override;

  bool start() override;
  void stop() override;
  bool resume() override;
  void suspend() override;
  void cleanup() override;

  [[nodiscard]] bool isRunning() const override;

 private:
  void run();
  /// Signal the worker to exit and join it. Safe to call repeatedly.
  void terminateWorker();

  std::function<void(DSPAudioBuffer *, int)> renderAudio_;
  std::shared_ptr<DSPAudioBuffer> buffer_;
  float sampleRate_;
  int channelCount_;
  std::atomic<bool> isInitialized_{false};
  std::atomic<bool> isRunning_{false};
  std::atomic<bool> isPaused_{true};
  std::atomic<bool> shouldStop_{false};
  std::thread worker_;
};

} // namespace audioapi
