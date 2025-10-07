#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>

#include <cmath>
#include <memory>
#include <string>

namespace audioapi {

class AudioBus;

class ConstantSourceNode : public AudioScheduledSourceNode {
 public:
  explicit ConstantSourceNode(BaseAudioContext *context);

  [[nodiscard]] std::shared_ptr<AudioParam> getOffsetParam() const;

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> offsetParam_;
};
} // namespace audioapi
