#pragma once

#include <memory>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <atomic>

#include "AudioNode.h"

namespace audioapi {

// TODO implement AudioScheduledSourceNode

class AudioScheduledSourceNode : public AudioNode {
 public:
  explicit AudioScheduledSourceNode(AudioContext *context);

  void start(double time);
  void stop(double time);
  void cleanup() override;

 protected:
  std::atomic<bool> isPlaying_;

private:
    void startPlayback();
    void stopPlayback();
    void waitAndExecute(double time, const std::function<void(double)>& fun);
};

} // namespace audioapi
