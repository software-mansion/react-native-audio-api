#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Constants.h"
#include "ChannelCountMode.h"
#include "ChannelInterpretation.h"

// channelCount always equal to 2

namespace audioapi {

class BaseAudioContext;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(BaseAudioContext *context);
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
  BaseAudioContext *context_;
  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  int channelCount_ = CHANNEL_COUNT;
  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ =
      ChannelInterpretation::SPEAKERS;

  std::vector<std::shared_ptr<AudioNode>> inputNodes_ = {};
  std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};

 private:
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

  void cleanup();
};

} // namespace audioapi
