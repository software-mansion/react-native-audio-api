#pragma once

#include <memory>

#include "AudioBuffer.h"
#include "AudioScheduledSourceNode.h"

namespace audioapi {

class AudioBus;

class AudioBufferSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferSourceNode(BaseAudioContext *context);

  [[nodiscard]] bool getLoop() const;
  [[nodiscard]] std::shared_ptr<AudioBuffer> getBuffer() const;
  void setLoop(bool loop);
  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  bool loop_;
  std::shared_ptr<AudioBuffer> buffer_;
  int bufferIndex_;
};

} // namespace audioapi
