#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>

#include <algorithm>
#include <memory>
#include <cassert>

namespace audioapi {

class AudioBus;

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(BaseAudioContext *context);

  [[nodiscard]] std::shared_ptr<AudioParam> getPanParam() const;

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
