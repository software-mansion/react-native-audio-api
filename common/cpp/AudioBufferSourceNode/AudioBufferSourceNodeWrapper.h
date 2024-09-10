#pragma once

#include <memory>
#include "AudioBufferWrapper.h"
#include "AudioScheduledSourceNodeWrapper.h"

#ifdef ANDROID
#include "AudioBufferSourceNode.h"
#else
#include "IOSAudioBufferSourceNode.h"
#endif

namespace audioapi {

#ifdef ANDROID
class AudioBufferSourceNode;
#endif

class AudioBufferSourceNodeWrapper : public AudioScheduledSourceNodeWrapper {
#ifdef ANDROID

 private:
  AudioBufferSourceNode *getAudioBufferSourceNodeFromAudioNode();

 public:
  explicit AudioBufferSourceNodeWrapper(AudioBufferSourceNode *audioBufferNode)
      : AudioScheduledSourceNodeWrapper(audioBufferNode) {}
#else

 private:
  std::shared_ptr<IOSAudioBufferSourceNode>
  getAudioBufferSourceNodeFromAudioNode();

 public:
  AudioBufferSourceNodeWrapper(
      std::shared_ptr<IOSAudioBufferSourceNode> bufferSourceNode)
      : AudioNodeWrapper(bufferSourceNode) {}
#endif

 public:
  void setLoop(bool loop);
  bool getLoop();
  std::shared_ptr<AudioBufferWrapper> getBuffer();
  void setBuffer(const std::shared_ptr<AudioBufferWrapper> &buffer);
};
} // namespace audioapi
