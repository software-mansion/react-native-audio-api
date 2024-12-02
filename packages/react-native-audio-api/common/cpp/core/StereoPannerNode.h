#pragma once

#include <algorithm>
#include <memory>

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class AudioBus;

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(BaseAudioContext *context);

  [[nodiscard]] std::shared_ptr<AudioParam> getPanParam() const;

 protected:
  void processNode(AudioBus *processingBus, int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
