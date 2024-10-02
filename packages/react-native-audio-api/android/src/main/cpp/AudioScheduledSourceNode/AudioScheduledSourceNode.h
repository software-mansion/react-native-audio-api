#pragma once

#include <memory>

#include "AudioNode.h"

namespace audioapi {

using namespace oboe;

class AudioScheduledSourceNode : public AudioNode {
 public:
  explicit AudioScheduledSourceNode();

  void start(double time);
  void stop(double time);
  void cleanup();

 protected:
  std::shared_ptr<oboe::AudioStream> mStream;

  static int constexpr sampleRate = 44100;
};

} // namespace audioapi
