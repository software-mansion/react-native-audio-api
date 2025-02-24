#pragma once

#include <memory>

#include "AudioNode.h"
#include "AudioParam.h"

namespace audioapi {

class AudioBus;

class GainNode : public AudioNode {
 public:
  explicit GainNode(BaseAudioContext *context);

  [[nodiscard]] std::shared_ptr<AudioParam> getGainParam() const;

 protected:
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> gainParam_;
};

} // namespace audioapi
