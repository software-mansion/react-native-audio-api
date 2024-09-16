#pragma once

#include <memory>
#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "StereoPannerNode.h"
#else
#include "IOSAudioContext.h"
#include "IOSStereoPannerNode.h"
#endif

namespace audioapi {

#ifdef ANDROID
class StereoPannerNode;
#endif

class StereoPannerNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID
 public:
  explicit StereoPannerNodeWrapper(StereoPannerNode *pannerNode);
#else
 private:
  std::shared_ptr<IOSStereoPannerNode> getStereoPannerNodeFromAudioNode();

 public:
  StereoPannerNodeWrapper(
      std::shared_ptr<IOSStereoPannerNode> stereoPannerNode);
#endif
 private:
  std::shared_ptr<AudioParamWrapper> panParam_;

 public:
  std::shared_ptr<AudioParamWrapper> getPanParam() const;
};
} // namespace audioapi
