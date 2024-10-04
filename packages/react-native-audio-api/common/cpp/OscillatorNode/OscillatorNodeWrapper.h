#pragma once

#include <memory>
#include <string>

#include "AudioParamWrapper.h"
#include "AudioScheduledSourceNodeWrapper.h"

#ifdef ANDROID
#include "OscillatorNode.h"
#else
#include "IOSOscillatorNode.h"
#endif

namespace audioapi {

class OscillatorNodeWrapper : public AudioScheduledSourceNodeWrapper {
#ifdef ANDROID

public:
  explicit OscillatorNodeWrapper(
      const std::shared_ptr<OscillatorNode> &oscillatorNode);

private:
    std::shared_ptr<OscillatorNode> getOscillatorNodeFromAudioNode();
#else

public:
  explicit OscillatorNodeWrapper(
      std::shared_ptr<IOSOscillatorNode> oscillatorNode);

private:
  std::shared_ptr<IOSOscillatorNode> getOscillatorNodeFromAudioNode();
#endif

 public:
  std::shared_ptr<AudioParamWrapper> getFrequencyParam() const;
  std::shared_ptr<AudioParamWrapper> getDetuneParam() const;
  std::string getType();
  void setType(const std::string &type);

private:
    std::shared_ptr<AudioParamWrapper> frequencyParam_;
    std::shared_ptr<AudioParamWrapper> detuneParam_;
};
} // namespace audioapi
