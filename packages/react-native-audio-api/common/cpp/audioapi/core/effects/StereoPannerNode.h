#pragma once

#include <audioapi/core/core/AudioNode.h>
#include <audioapi/core/core/AudioParam.h>

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
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
