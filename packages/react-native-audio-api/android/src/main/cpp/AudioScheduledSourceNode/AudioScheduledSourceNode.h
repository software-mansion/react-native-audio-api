#pragma once

#include <memory>

#include "AudioNode.h"

namespace audioapi {

using namespace oboe;

//TODO implement AudioScheduledSourceNode

class AudioScheduledSourceNode : public AudioNode {
 public:
  explicit AudioScheduledSourceNode(AudioContext *context);

  void start(double time);
  void stop(double time);
  void cleanup() override;

 protected:
  std::shared_ptr<oboe::AudioStream> mStream;

  static int constexpr sampleRate = 44100;
};

} // namespace audioapi
