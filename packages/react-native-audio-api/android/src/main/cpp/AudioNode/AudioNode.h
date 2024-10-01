#pragma once

#include <fbjni/fbjni.h>
#include <react/jni/CxxModuleWrapper.h>
#include <react/jni/JMessageQueueThread.h>
#include <memory>
#include <string>

namespace audioapi {

class AudioNode {
 protected:
  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  int channelCount_ = 2;
  std::string channelCountMode_ = "max";
  std::string channelInterpretation_ = "speakers";

 public:
  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNode> &node) const;
  void disconnect(const std::shared_ptr<AudioNode> &node) const;
};

} // namespace audioapi
