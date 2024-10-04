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

public:
  explicit AudioNodeWrapper(const std::shared_ptr<AudioNode> &node);

protected:
    std::shared_ptr<AudioNode> node_;
#else

public:
  explicit AudioNodeWrapper(std::shared_ptr<IOSAudioNode> node);

protected:
    std::shared_ptr<IOSAudioNode> node_;
#endif

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
