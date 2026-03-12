#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cassert>
#include <memory>

namespace audioapi {

struct StereoPannerOptions;

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const StereoPannerOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getPanParam() const;

 protected:
  std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  const std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
