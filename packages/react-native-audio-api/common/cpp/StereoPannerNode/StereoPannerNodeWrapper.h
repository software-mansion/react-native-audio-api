#pragma once

#include <memory>
#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"

#ifdef ANDROID
#include "StereoPannerNode.h"
#else
#include "IOSStereoPannerNode.h"
#endif

namespace audioapi {

class StereoPannerNodeWrapper : public AudioNodeWrapper {
#ifdef ANDROID

 public:
  explicit StereoPannerNodeWrapper(
      const std::shared_ptr<StereoPannerNode> &stereoPannerNode);
#else

 public:
  StereoPannerNodeWrapper(
      const std::shared_ptr<IOSStereoPannerNode> &stereoPannerNode);
#endif

public:
  std::shared_ptr<AudioParamWrapper> getPanParam() const;

private:
    std::shared_ptr<AudioParamWrapper> panParam_;
};
} // namespace audioapi
