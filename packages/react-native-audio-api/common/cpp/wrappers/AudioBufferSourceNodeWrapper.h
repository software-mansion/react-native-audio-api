#pragma once

#include <memory>

#include "AudioBufferWrapper.h"
#include "AudioScheduledSourceNodeWrapper.h"
#include "AudioBufferSourceNode.h"

namespace audioapi {

class AudioBufferSourceNodeWrapper : public AudioScheduledSourceNodeWrapper {

 public:
  explicit AudioBufferSourceNodeWrapper(
      const std::shared_ptr<AudioBufferSourceNode> &audioBufferSourceNode);

  void setLoop(bool loop);
  bool getLoop();
  std::shared_ptr<AudioBufferWrapper> getBuffer();
  void setBuffer(const std::shared_ptr<AudioBufferWrapper> &buffer);

private:
    std::shared_ptr<AudioBufferSourceNode>
    getAudioBufferSourceNodeFromAudioNode();
};
} // namespace audioapi
