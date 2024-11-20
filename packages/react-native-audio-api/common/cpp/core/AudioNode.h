#pragma once

#include <memory>
#include <string>
#include <vector>
#include "Constants.h"

namespace audioapi {

class BaseAudioContext;
class AudioBus;

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

 protected:
  enum class ChannelCountMode { MAX, CLAMPED_MAX, EXPLICIT };
  enum class ChannelInterpretation { SPEAKERS, DISCRETE };

  static std::string toString(ChannelCountMode mode);
  static std::string toString(ChannelInterpretation interpretation);

  BaseAudioContext *context_;
  std::shared_ptr<AudioBus> audioBus_;

  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  int channelCount_ = CHANNEL_COUNT;
  std::string debugName_;
  bool isInitialized_ = false;

  std::size_t lastRenderedFrame_ { SIZE_MAX };

  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ =
      ChannelInterpretation::SPEAKERS;

  std::vector<std::shared_ptr<AudioNode>> inputNodes_ = {};
  std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};


  void cleanup();
  AudioBus* processAudio(AudioBus* outputBus, int framesToProcess);
  virtual void processNode(AudioBus* processingBus, int framesToProcess) = 0;
};

} // namespace audioapi
