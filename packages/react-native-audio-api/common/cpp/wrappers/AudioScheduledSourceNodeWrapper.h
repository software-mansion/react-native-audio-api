#pragma once

#include <memory>

#include "AudioNodeWrapper.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

class AudioScheduledSourceNodeWrapper : public AudioNodeWrapper {

 public:
  explicit AudioScheduledSourceNodeWrapper(
      const std::shared_ptr<AudioScheduledSourceNode>
          &audioScheduledSourceNode);

    void start(double time);
    void stop(double time);

 private:
  std::shared_ptr<AudioScheduledSourceNode>
  getAudioScheduledSourceNodeFromAudioNode();
};
} // namespace audioapi
