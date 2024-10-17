#pragma once

#include <memory>
#include <string>

#include "AudioParamWrapper.h"
#include "AudioScheduledSourceNodeWrapper.h"
#include "OscillatorNode.h"

namespace audioapi {

class OscillatorNodeWrapper : public AudioScheduledSourceNodeWrapper {

 public:
  explicit OscillatorNodeWrapper(
      const std::shared_ptr<OscillatorNode> &oscillatorNode);

    std::shared_ptr<AudioParamWrapper> getFrequencyParam() const;
    std::shared_ptr<AudioParamWrapper> getDetuneParam() const;
    std::string getType();
    void setType(const std::string &type);

 private:
  std::shared_ptr<AudioParamWrapper> frequencyParam_;
  std::shared_ptr<AudioParamWrapper> detuneParam_;

    std::shared_ptr<OscillatorNode> getOscillatorNodeFromAudioNode();
};
} // namespace audioapi
