#pragma once

#include <memory>

#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "AudioScheduledSourceNode.h"
#else
#include "IOSAudioScheduledSourceNode.h"
#endif

namespace audioapi {

class AudioScheduledSourceNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID

public:
  explicit AudioScheduledSourceNodeWrapper(
      const std::shared_ptr<AudioScheduledSourceNode> &audioScheduledSourceNode);

private:
    std::shared_ptr<AudioScheduledSourceNode>
    getAudioScheduledSourceNodeFromAudioNode();
#else

public:
  AudioScheduledSourceNodeWrapper(
      std::shared_ptr<IOSAudioScheduledSourceNode> audioScheduledSourceNode);

private:
  std::shared_ptr<IOSAudioScheduledSourceNode>
  getAudioScheduledSourceNodeFromAudioNode();
#endif

 public:
  void start(double time);
  void stop(double time);
};
} // namespace audioapi
