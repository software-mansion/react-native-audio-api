#pragma once

#include <memory>
#include <cstddef>
#include <string>

#include "signalsmith-stretch.h"
#include "AudioBuffer.h"
#include "AudioScheduledSourceNode.h"
#include "TimeStretchType.h"

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
  [[nodiscard]] std::shared_ptr<AudioBuffer> getBuffer() const;
  [[nodiscard]] std::string getTimeStretch() const;

  void setLoop(bool loop);
  void setLoopStart(double loopStart);
  void setLoopEnd(double loopEnd);
  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);
  void setTimeStretch(const std::string& timeStretchType);

  void start(double when, double offset, double duration = -1);

 protected:
  std::mutex &getBufferLock();
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  // Looping related properties
  bool loop_;
  double loopStart_;
  double loopEnd_;
  std::mutex bufferLock_;

  // playback rate aka pitch change params
  std::shared_ptr<AudioParam> detuneParam_;
  std::shared_ptr<AudioParam> playbackRateParam_;
  TimeStretchType timeStretch_ = TimeStretchType::LINEAR;

  // internal helper
  double vReadIndex_;

  // User provided buffer
  std::shared_ptr<AudioBuffer> buffer_;
  std::shared_ptr<AudioBus> alignedBus_;

  // time stretching
  std::shared_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretch_;
  std::shared_ptr<AudioBus> playbackRateBus_;

  float getPlaybackRateValue(size_t &startOffset);

  double getVirtualStartFrame();
  double getVirtualEndFrame();

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

    static TimeStretchType fromString(const std::string &type) {
        std::string lowerType = type;
        std::transform(
                lowerType.begin(), lowerType.end(), lowerType.begin(), ::tolower);

        if (lowerType == "linear")
            return TimeStretchType::LINEAR;
        if (lowerType == "speech")
            return TimeStretchType::SPEECH;
        if (lowerType == "music")
            return TimeStretchType::MUSIC;

        throw std::invalid_argument("Unknown time stretch type: " + type);
    }

    static std::string toString(TimeStretchType type) {
        switch (type) {
            case TimeStretchType::LINEAR:
                return "linear";
            case TimeStretchType::SPEECH:
                return "speech";
            case TimeStretchType::MUSIC:
                return "music";
            default:
                throw std::invalid_argument("Unknown time stretch type");
        }
    }
};

} // namespace audioapi
