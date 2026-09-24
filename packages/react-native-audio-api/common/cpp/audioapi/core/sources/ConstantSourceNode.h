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

  /// @brief The output stays mono whatever the `channelCount` attribute says;
  /// the attribute is tracked by the host object only.
  void setChannelCount(size_t /*channelCount*/) override {}

 protected:
  void processNode(int framesToProcess) override;

 private:
  const std::shared_ptr<AudioParam> offsetParam_;
};
} // namespace audioapi
