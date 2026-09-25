#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {

struct ConstantSourceOptions;

class ConstantSourceNode : public AudioScheduledSourceNode {
 public:
  explicit ConstantSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const ConstantSourceOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getOffsetParam() const;

 protected:
  void processNode(int framesToProcess) override;

 private:
  const std::shared_ptr<AudioParam> offsetParam_;
};
} // namespace audioapi
