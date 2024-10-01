#pragma once

#include <memory>
#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "AudioScheduledSourceNode.h"
#include "OscillatorNode.h"
#else
#include "IOSAudioScheduledSourceNode.h"
#endif

namespace audioapi {

#ifdef ANDROID
class AudioScheduledSourceNode;
#endif

class AudioScheduledSourceNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID

 private:
  std::shared_ptr<AudioScheduledSourceNode>
  getAudioScheduledSourceNodeFromAudioNode();

 public:
  explicit AudioScheduledSourceNodeWrapper(
      const std::shared_ptr<AudioScheduledSourceNode> &audioScheduledSourceNode)
      : AudioNodeWrapper(audioScheduledSourceNode) {}
#else

 private:
  std::shared_ptr<IOSAudioScheduledSourceNode>
  getAudioScheduledSourceNodeFromAudioNode();

 public:
  AudioScheduledSourceNodeWrapper(
      std::shared_ptr<IOSAudioScheduledSourceNode> audioScheduledSourceNode)
      : AudioNodeWrapper(audioScheduledSourceNode) {}
#endif

 public:
  void start(double time);
  void stop(double time);
};
} // namespace audioapi
