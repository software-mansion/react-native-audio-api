#pragma once

#include <oboe/Oboe.h>
#include "AudioNode.h"

namespace audioapi {

using namespace oboe;

class AudioScheduledSourceNode : public AudioNode {
 protected:
  std::shared_ptr<oboe::AudioStream> mStream;

  static int constexpr sampleRate = 44100;

 public:
  explicit AudioScheduledSourceNode();

  void start(double time);
  void stop(double time);
};

} // namespace audioapi
