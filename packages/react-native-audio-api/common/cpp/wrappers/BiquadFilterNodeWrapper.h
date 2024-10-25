#pragma once

#include <memory>
#include <string>

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"
#include "BiquadFilterNode.h"

namespace audioapi {

class BiquadFilterNodeWrapper : public AudioNodeWrapper {
 public:
  explicit BiquadFilterNodeWrapper(
      const std::shared_ptr<BiquadFilterNode> &biquadFilterNode);

  std::shared_ptr<AudioParamWrapper> getFrequencyParam() const;
  std::shared_ptr<AudioParamWrapper> getDetuneParam() const;
  std::shared_ptr<AudioParamWrapper> getQParam() const;
  std::shared_ptr<AudioParamWrapper> getGainParam() const;
  std::string getType();
  void setType(const std::string &filterType);
  void getFrequencyResponse(const std::vector<float> &frequencyArray,
                              std::vector<float> &magResponseOutput,
                              std::vector<float> &phaseResponseOutput);

 private:
  std::shared_ptr<AudioParamWrapper> frequencyParam_;
  std::shared_ptr<AudioParamWrapper> detuneParam_;
  std::shared_ptr<AudioParamWrapper> QParam_;
  std::shared_ptr<AudioParamWrapper> gainParam_;

  std::shared_ptr<BiquadFilterNode> getBiquadFilterNodeFromAudioNode();
};
} // namespace audioapi
