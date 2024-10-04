#pragma once

#include <memory>
#include <string>

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "BiquadFilterNode.h"
#else
#include "IOSBiquadFilterNode.h"
#endif

namespace audioapi {

class BiquadFilterNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID

public:
  explicit BiquadFilterNodeWrapper(
      const std::shared_ptr<BiquadFilterNode> &biquadFilterNode);

private:
    std::shared_ptr<BiquadFilterNode> getBiquadFilterNodeFromAudioNode();
#else

public:
  explicit BiquadFilterNodeWrapper(
      std::shared_ptr<IOSBiquadFilterNode> biquadFilterNode);

private:
  std::shared_ptr<IOSBiquadFilterNode> getBiquadFilterNodeFromAudioNode();
#endif

 public:
  std::shared_ptr<AudioParamWrapper> getFrequencyParam() const;
  std::shared_ptr<AudioParamWrapper> getDetuneParam() const;
  std::shared_ptr<AudioParamWrapper> getQParam() const;
  std::shared_ptr<AudioParamWrapper> getGainParam() const;
  std::string getType();
  void setType(const std::string &filterType);

private:
    std::shared_ptr<AudioParamWrapper> frequencyParam_;
    std::shared_ptr<AudioParamWrapper> detuneParam_;
    std::shared_ptr<AudioParamWrapper> QParam_;
    std::shared_ptr<AudioParamWrapper> gainParam_;
};
} // namespace audioapi
