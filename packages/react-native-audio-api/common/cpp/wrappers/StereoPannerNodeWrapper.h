#pragma once

#include <memory>

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"
#include "StereoPannerNode.h"

namespace audioapi {

class StereoPannerNodeWrapper : public AudioNodeWrapper {

 public:
  explicit StereoPannerNodeWrapper(
      const std::shared_ptr<StereoPannerNode> &stereoPannerNode);

  std::shared_ptr<AudioParamWrapper> getPanParam() const;

 private:
  std::shared_ptr<AudioParamWrapper> panParam_;
};
} // namespace audioapi
