#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/types/TimeStretchType.h>

#include <memory>
#include <cstddef>
#include <algorithm>
#include <string>

namespace audioapi {

class AudioBus;
class AudioParam;

class AudioBufferSourceNode : public AudioScheduledSourceNode {
 public:
  explicit AudioBufferSourceNode(BaseAudioContext *context);

  [[nodiscard]] bool getLoop() const;
  [[nodiscard]] double getLoopStart() const;
  [[nodiscard]] double getLoopEnd() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getDetuneParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getPlaybackRateParam() const;
  [[nodiscard]] std::shared_ptr<AudioParam> getSemitonesParam() const;
  [[nodiscard]] std::shared_ptr<AudioBuffer> getBuffer() const;
  [[nodiscard]] std::string getTimeStretchType() const;

  void setLoop(bool loop);
  void setLoopStart(double loopStart);
  void setLoopEnd(double loopEnd);
  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);
  void setTimeStretchType(const std::string &type);

  void start(double when, double offset, double duration = -1);

 protected:
  std::mutex &getBufferLock();
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
  double getStopTime() const override;

 private:
  static TimeStretchType fromString(const std::string &type) {
    std::string lowerType = type;
    std::transform(
        lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

    if (lowerType == "linear")
      return TimeStretchType::LINEAR;
    if (lowerType == "speech-music")
        return TimeStretchType::SPEECH_MUSIC;

    throw std::invalid_argument("Unknown time stretch type: " + type);
  }

  static std::string toString(TimeStretchType type) {
    switch (type) {
      case TimeStretchType::LINEAR:
        return "linear";
      case TimeStretchType::SPEECH_MUSIC:
        return "speech-music";
    }

    throw std::invalid_argument("Unknown time stretch type");
  }

  // Looping related properties
  bool loop_;
  double loopStart_;
  double loopEnd_;
  std::mutex bufferLock_;

  // time-stretching and pitch-shifting
  TimeStretchType timeStretchType_;

  // k-rate params
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> playbackRateParam_;
  std::shared_ptr<AudioParam> semitones_;

  std::shared_ptr<AudioBus> playbackRateBus_;

  // internal helper
  double vReadIndex_;

  // User provided buffer
  std::shared_ptr<AudioBuffer> buffer_;

  void processWithoutInterpolation(
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);

  void processWithInterpolation(
      const std::shared_ptr<AudioBus>& processingBus,
      size_t startOffset,
      size_t offsetLength,
      float playbackRate);

  float getPlaybackRateValue();

  double getVirtualStartFrame();
  double getVirtualEndFrame();
};

} // namespace audioapi
