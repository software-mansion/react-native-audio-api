#pragma once

#include <memory>
#include <string>

#ifdef ANDROID
#include "AudioNode.h"
#else
#include "IOSAudioNode.h"
#endif

namespace audioapi {
class AudioNodeWrapper {
#ifdef ANDROID

 protected:
  AudioNode *node_;

 public:
  explicit AudioNodeWrapper(AudioNode *node);
  virtual ~AudioNodeWrapper();
#else

 public:
  std::shared_ptr<IOSAudioNode> node_;
    explicit AudioNodeWrapper(std::shared_ptr<IOSAudioNode> node);
#endif

 private:
  int numberOfInputs_;
  int numberOfOutputs_;

 public:
  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNodeWrapper> &node) const;
  void disconnect(const std::shared_ptr<AudioNodeWrapper> &node) const;
};
} // namespace audioapi
