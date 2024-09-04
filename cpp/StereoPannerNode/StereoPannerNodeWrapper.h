#pragma once

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"
#include <memory>

#ifdef ANDROID
#include "StereoPannerNode.h"
#else
#include "IOSAudioContext.h"
#include "IOSStereoPannerNode.h"
#endif

namespace audiocontext {

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
  explicit StereoPannerNodeWrapper(std::shared_ptr<IOSAudioContext> context);
#endif
 private:
  std::shared_ptr<AudioParamWrapper> panParam_;

 public:
  std::shared_ptr<AudioParamWrapper> getPanParam() const;
};
} // namespace audiocontext
