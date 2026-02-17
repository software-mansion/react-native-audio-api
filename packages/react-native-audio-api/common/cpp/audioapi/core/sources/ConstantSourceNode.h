#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>

#include <cmath>
#include <memory>
#include <string>

namespace audioapi {

class AudioBuffer;
struct ConstantSourceOptions;

class ConstantSourceNode : public AudioScheduledSourceNode {
 public:
  explicit ConstantSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConstantSourceOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getOffsetParam() const;

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> offsetParam_;
};
} // namespace audioapi
