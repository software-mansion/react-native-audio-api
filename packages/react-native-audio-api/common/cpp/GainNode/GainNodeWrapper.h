#pragma once

#include <memory>

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "GainNode.h"
#else
#include "IOSGainNode.h"
#endif

namespace audioapi {

class GainNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID
 public:
  explicit GainNodeWrapper(const std::shared_ptr<GainNode> &gainNode);
#else

 public:
  explicit GainNodeWrapper(const std::shared_ptr<IOSGainNode> &gainNode);

 private:
  std::shared_ptr<IOSGainNode> getGainNodeFromAudioNode();
#endif

 public:
  std::shared_ptr<AudioParamWrapper> getGainParam() const;

 private:
  std::shared_ptr<AudioParamWrapper> gainParam_;
};
} // namespace audioapi
