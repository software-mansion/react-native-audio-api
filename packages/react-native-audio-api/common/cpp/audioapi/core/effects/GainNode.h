#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>

#include <memory>

namespace audioapi {

class AudioBus;
struct GainOptions;

class GainNode : public AudioNode {
 public:
  explicit GainNode(const std::shared_ptr<BaseAudioContext> &context, const GainOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getGainParam() const;

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> gainParam_;
};

} // namespace audioapi
