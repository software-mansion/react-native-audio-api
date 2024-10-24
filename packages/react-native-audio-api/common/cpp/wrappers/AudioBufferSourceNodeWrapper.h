#pragma once

#include <memory>

#include "AudioBufferSourceNode.h"
#include "AudioBufferWrapper.h"
#include "AudioScheduledSourceNodeWrapper.h"

namespace audioapi {

class AudioBufferSourceNodeWrapper : public AudioScheduledSourceNodeWrapper {
 public:
  explicit AudioBufferSourceNodeWrapper(
      const std::shared_ptr<AudioBufferSourceNode> &audioBufferSourceNode);

  void setLoop(bool loop);
  [[nodiscard]] bool getLoop();
  [[nodiscard]] std::shared_ptr<AudioBufferWrapper> getBuffer();
  void setBuffer(const std::shared_ptr<AudioBufferWrapper> &buffer);

 private:
  std::shared_ptr<AudioBufferSourceNode>
  getAudioBufferSourceNodeFromAudioNode();
};
} // namespace audioapi
