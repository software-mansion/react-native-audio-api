#pragma once

#include <audioapi/core/AudioPlayer.h>

#include <atomic>
#include <optional>
#include <thread>

namespace audioapi {

class NodeAudioPlayer final : public AudioPlayer {
 public:
  using AudioPlayer::AudioPlayer;
  ~NodeAudioPlayer() override;

  DELETE_COPY_AND_MOVE(NodeAudioPlayer);

  bool start() override;
  void stop() override;
  bool resume() override;
  void suspend() override;
  void cleanup() override;

  [[nodiscard]] bool isRunning() const override;
  [[nodiscard]] double getOutputLatency() const override;
  [[nodiscard]] double getBaseLatency() const override;

  /// The virtual device plays a quantum at the deadline the render loop paced it to, with no
  /// output latency between the two.
  [[nodiscard]] std::optional<OutputPresentation> getOutputPresentation() override;

 private:
  void run();
  /// Signal the worker to exit and join it. Safe to call repeatedly.
  void terminateWorker();

  std::atomic<bool> isInitialized_{false};
  std::atomic<bool> isPaused_{true};
  std::atomic<bool> shouldStop_{false};
  std::thread worker_;
};

} // namespace audioapi
