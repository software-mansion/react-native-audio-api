#pragma once

#include <oboe/Oboe.h>
#include <memory>
#include <string>
#include <vector>

// channelCount always equal to 2

namespace audioapi {

class AudioContext;

using namespace oboe;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(AudioContext *context);
  virtual ~AudioNode() = default;
  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNode> &node);
  void disconnect(const std::shared_ptr<AudioNode> &node);

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
  int channelCount_ = 2;
  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ =
      ChannelInterpretation::SPEAKERS;

  virtual void process(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames,
      int channelCount);

  virtual void cleanup();

 private:
  // TODO check
  std::vector<std::shared_ptr<AudioNode>> inputNodes_ = {};
  std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};
};

} // namespace audioapi
