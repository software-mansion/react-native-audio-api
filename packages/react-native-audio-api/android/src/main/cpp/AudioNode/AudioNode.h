#pragma once

#include <oboe/Oboe.h>
#include <memory>
#include <string>
#include <vector>

// channelCount always equal to 2

namespace audioapi {

using namespace oboe;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
    explicit AudioNode();
  virtual ~AudioNode() = default;
  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNode> &node);
  void disconnect(const std::shared_ptr<AudioNode> &node);

 protected:
    //std::weak_ptr<AudioContext> context_;
  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  int channelCount_ = 2;
  // TODO: Add enum for channelCountMode
  std::string channelCountMode_ = "max";
  // TODO: Add enum for channelInterpretation
  std::string channelInterpretation_ = "speakers";

  virtual void process(
      AudioStream *oboeStream,
      void *audioData,
      int32_t numFrames,
      int channelCount);

 private:
  std::vector<std::shared_ptr<AudioNode>> inputNodes_ = {};
  std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};
};

} // namespace audioapi
