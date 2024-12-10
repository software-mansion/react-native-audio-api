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
  [[nodiscard]] double getLoopStart() const;
  [[nodiscard]] double getLoopEnd() const;

  [[nodiscard]] double getDetune() const;
  [[nodiscard]] double getPlaybackRate() const;

  [[nodiscard]] std::shared_ptr<AudioBuffer> getBuffer() const;

  void setLoop(bool loop);
  void setLoopStart(double loopStart);
  void setLoopEnd(double loopEnd);

  void setDetune(double detune);
  void setPlaybackRate(double playbackRate);

  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  // Looping related properties
  bool loop_;
  double loopEnd_;
  double loopStart_;

  // playback rate aka pitch change properties
  double detune_;
  double playbackRate_;

  // internal helpers
  int bufferIndex_;

  // User provided buffer
  std::shared_ptr<AudioBuffer> buffer_;
};

} // namespace audioapi
