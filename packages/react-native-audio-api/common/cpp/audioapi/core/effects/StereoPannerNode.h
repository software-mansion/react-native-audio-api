#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>

#include <algorithm>
#include <cassert>
#include <memory>

namespace audioapi {

class AudioBus;
struct StereoPannerOptions;

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const StereoPannerOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getPanParam() const;

 protected:
  std::shared_ptr<AudioBus> processNode(
      const std::shared_ptr<AudioBus> &processingBus,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> panParam_;
};

} // namespace audioapi
