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

class AudioBufferSourceNodeWrapper : public AudioScheduledSourceNodeWrapper {
#ifdef ANDROID

public:
  explicit AudioBufferSourceNodeWrapper(
      const std::shared_ptr<AudioBufferSourceNode> &audioBufferSourceNode);

private:
    std::shared_ptr<AudioBufferSourceNode>
    getAudioBufferSourceNodeFromAudioNode();
#else

public:
  AudioBufferSourceNodeWrapper(
      std::shared_ptr<IOSAudioBufferSourceNode> bufferSourceNode);

private:
  std::shared_ptr<IOSAudioBufferSourceNode>
  getAudioBufferSourceNodeFromAudioNode();
#endif

 public:
  void setLoop(bool loop);
  bool getLoop();
  std::shared_ptr<AudioBufferWrapper> getBuffer();
  void setBuffer(const std::shared_ptr<AudioBufferWrapper> &buffer);
};
} // namespace audioapi
