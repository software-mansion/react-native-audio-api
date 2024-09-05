#pragma once

#include <memory>
#include "AudioBufferWrapper.h"
#include "AudioNodeWrapper.h"

#ifdef ANDROID
#include "AudioBufferSourceNode.h"
#else
#endif

namespace audioapi {

#ifdef ANDROID
class AudioBufferSourceNode;
#endif

class AudioBufferSourceNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID

 private:
  AudioBufferSourceNode *getAudioBufferSourceNodeFromAudioNode();

 public:
  explicit AudioBufferSourceNodeWrapper(AudioBufferSourceNode *audioBufferNode)
      : AudioNodeWrapper(audioBufferNode) {}
#else

 public:
  AudioBufferSourceNodeWrapper() : AudioNodeWrapper() {
    node_ = std::make_shared<AudioNode>(context);
  }
#endif

 public:
  void start(double time);
  void stop(double time);
  void setLoop(bool loop);
  bool getLoop();
  std::shared_ptr<AudioBufferWrapper> getBuffer();
  void setBuffer(const std::shared_ptr<AudioBufferWrapper> &buffer);
};
} // namespace audioapi
