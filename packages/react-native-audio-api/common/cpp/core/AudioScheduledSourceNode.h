#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <thread>

#include "AudioNode.h"

namespace audioapi {

class AudioScheduledSourceNode : public AudioNode {
 public:
  enum class PlaybackState { UNSCHEDULED, SCHEDULED, PLAYING, FINISHED };
  explicit AudioScheduledSourceNode(BaseAudioContext *context);

  void start(double time);
  void stop(double time);

  bool isFinished();
  bool isPlaying();

 protected:
  std::atomic<PlaybackState> playbackState_;

 private:
  void startPlayback();
  void stopPlayback();
  void waitAndExecute(double time, const std::function<void(double)> &fun);
};

} // namespace audioapi
