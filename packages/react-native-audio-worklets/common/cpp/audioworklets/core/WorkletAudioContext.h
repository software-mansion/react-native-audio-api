#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioapi/events/IAudioEventHandlerRegistry.h>

#include <memory>
#include <mutex>
#include <thread>

namespace audioworklets {

class WorkletAudioContext : public audioapi::BaseAudioContext {
 public:
  explicit WorkletAudioContext(
      float sampleRate,
      const std::shared_ptr<audioapi::IAudioEventHandlerRegistry> &audioEventHandlerRegistry);
  ~WorkletAudioContext() override;
  DELETE_COPY_AND_MOVE(WorkletAudioContext);

  void close();
  bool resume();
  bool suspend();

  void initialize(const audioapi::AudioDestinationNode *destination) final;

 private:
  std::shared_ptr<audioapi::DSPAudioBuffer> buffer_;
  std::thread worker_;
  bool shouldStop_{false};
  bool isInitialized_{false};

  /// Thread entry point; keeps the thread JVM-attached on Android.
  void run();
  void renderLoop();
  bool isDriverRunning() const override;
};

} // namespace audioworklets
