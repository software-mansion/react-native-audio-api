#pragma once

#include <memory>

#include "AudioNodeWrapper.h"
#include "AudioParamWrapper.h"
#include "GainNode.h"

namespace audioapi {

class GainNodeWrapper : public AudioNodeWrapper {
 public:
  explicit GainNodeWrapper(const std::shared_ptr<GainNode> &gainNode);

  std::shared_ptr<AudioParamWrapper> getGainParam() const;

 private:
  std::shared_ptr<AudioParamWrapper> gainParam_;
};
} // namespace audioapi
