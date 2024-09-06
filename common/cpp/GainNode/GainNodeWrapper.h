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

#ifdef ANDROID
class GainNode;
#endif

class GainNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID
 public:
  explicit GainNodeWrapper(GainNode *gainNode);
#else
 public:
  explicit GainNodeWrapper(std::shared_ptr<IOSGainNode> gainNode);
#endif
 private:
  std::shared_ptr<AudioParamWrapper> gainParam_;

 public:
  std::shared_ptr<AudioParamWrapper> getGainParam() const;
};
} // namespace audioapi
