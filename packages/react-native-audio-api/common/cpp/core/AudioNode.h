#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Constants.h"

// channelCount always equal to 2

namespace audioapi {

class AudioContext;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(AudioContext *context);
  virtual ~AudioNode();
  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNode> &node);
  void disconnect(const std::shared_ptr<AudioNode> &node);

  // Change public to protected
  virtual bool processAudio(float *audioData, int32_t numFrames);

 protected:
  enum class ChannelCountMode { MAX, CLAMPED_MAX, EXPLICIT };

  static std::string toString(ChannelCountMode mode) {
    switch (mode) {
      case ChannelCountMode::MAX:
        return "max";
      case ChannelCountMode::CLAMPED_MAX:
        return "clamped-max";
      case ChannelCountMode::EXPLICIT:
        return "explicit";
      default:
        throw std::invalid_argument("Unknown channel count mode");
    }
  }

  enum class ChannelInterpretation { SPEAKERS, DISCRETE };

  static std::string toString(ChannelInterpretation interpretation) {
    switch (interpretation) {
      case ChannelInterpretation::SPEAKERS:
        return "speakers";
      case ChannelInterpretation::DISCRETE:
        return "discrete";
      default:
        throw std::invalid_argument("Unknown channel interpretation");
    }
  }

 protected:
  AudioContext *context_;
  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  int channelCount_ = CHANNEL_COUNT;
  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ =
      ChannelInterpretation::SPEAKERS;

  std::vector<std::shared_ptr<AudioNode>> inputNodes_ = {};
  std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};

 private:
  void cleanup();
};

} // namespace audioapi
