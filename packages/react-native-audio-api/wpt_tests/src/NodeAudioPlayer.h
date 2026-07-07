#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

namespace audioapi {

class NodeAudioPlayer final {
 public:
  NodeAudioPlayer(
      const std::function<void(DSPAudioBuffer *, int)> &renderAudio,
      float sampleRate,
      int channelCount);
  ~NodeAudioPlayer();

  bool start();
  void stop();
  bool resume();
  void suspend();
  void cleanup();

  [[nodiscard]] bool isRunning() const;

 private:
  void run();

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
