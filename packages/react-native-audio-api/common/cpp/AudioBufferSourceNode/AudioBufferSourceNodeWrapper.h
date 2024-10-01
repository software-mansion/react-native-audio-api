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
  std::shared_ptr<AudioBufferSourceNode>
  getAudioBufferSourceNodeFromAudioNode();

 public:
  explicit AudioBufferSourceNodeWrapper(
      const std::shared_ptr<AudioBufferSourceNode> &audioBufferSourceNode)
      : AudioScheduledSourceNodeWrapper(audioBufferSourceNode) {}
#else

 private:
  std::shared_ptr<IOSAudioBufferSourceNode>
  getAudioBufferSourceNodeFromAudioNode();

 public:
  AudioBufferSourceNodeWrapper(
      std::shared_ptr<IOSAudioBufferSourceNode> bufferSourceNode)
      : AudioScheduledSourceNodeWrapper(bufferSourceNode) {}
#endif

 public:
  void setLoop(bool loop);
  bool getLoop();
  std::shared_ptr<AudioBufferWrapper> getBuffer();
  void setBuffer(const std::shared_ptr<AudioBufferWrapper> &buffer);
};
} // namespace audioapi
