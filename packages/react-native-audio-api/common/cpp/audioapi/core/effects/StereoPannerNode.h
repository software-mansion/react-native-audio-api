#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cassert>
#include <memory>
#include <optional>

namespace audioapi {

struct StereoPannerOptions;

class StereoPannerNode : public AudioNode {
 public:
  explicit StereoPannerNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const StereoPannerOptions &options);

  [[nodiscard]] std::shared_ptr<AudioParam> getPanParam() const;
  [[nodiscard]] std::shared_ptr<DSPAudioBuffer> getOutputBuffer() const override;
  [[nodiscard]] std::shared_ptr<DSPAudioBuffer> getNegotiatedBuffer() const override;
  void setNegotiatedBuffer(const std::shared_ptr<DSPAudioBuffer> &buffer) override;
  [[nodiscard]] std::optional<size_t> getOutputChannelNumber() const override;

 protected:
  void processNode(int framesToProcess) override;
  [[nodiscard]] const DSPAudioBuffer *getOutput() const override {
    return outputBuffer_.get();
  }

 private:
  static constexpr size_t kOutputChannelNumber = 2;

  const std::shared_ptr<AudioParam> panParam_;
  const std::shared_ptr<DSPAudioBuffer> outputBuffer_;
};

} // namespace audioapi
