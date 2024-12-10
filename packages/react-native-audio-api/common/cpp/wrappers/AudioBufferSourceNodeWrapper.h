#pragma once

#include <memory>
#include <optional>

#include "AudioBufferSourceNode.h"
#include "AudioBufferWrapper.h"
#include "AudioScheduledSourceNodeWrapper.h"

namespace audioapi {

class AudioParamWrapper;

class AudioBufferSourceNodeWrapper : public AudioScheduledSourceNodeWrapper {
 public:
  explicit AudioBufferSourceNodeWrapper(
      const std::shared_ptr<AudioBufferSourceNode> &audioBufferSourceNode);

  [[nodiscard]] bool getLoop();
  [[nodiscard]] double getLoopStart();
  [[nodiscard]] double getLoopEnd();

  [[nodiscard]] std::shared_ptr<AudioParamWrapper> getDetuneParam();
  [[nodiscard]] std::shared_ptr<AudioParamWrapper> getPlaybackRateParam();

  [[nodiscard]] std::shared_ptr<AudioBufferWrapper> getBuffer();

  void setLoop(bool loop);
  void setLoopStart(double loopStart);
  void setLoopEnd(double loopEnd);

  void setBuffer(const std::shared_ptr<AudioBufferWrapper> &buffer);

 private:
  std::shared_ptr<AudioBufferSourceNode>
  getAudioBufferSourceNodeFromAudioNode();

  std::shared_ptr<AudioParamWrapper> detuneParam_;
  std::shared_ptr<AudioParamWrapper> playbackRateParam_;
};
} // namespace audioapi
