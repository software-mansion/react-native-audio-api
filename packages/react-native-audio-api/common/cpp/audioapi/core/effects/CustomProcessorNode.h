#pragma once

#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>

#include <memory>
#include <string>

namespace audioapi {

class AudioBus;

class CustomProcessorNode : public AudioNode {
 public:
  explicit CustomProcessorNode(BaseAudioContext *context);

  [[nodiscard]] std::shared_ptr<AudioParam> getCustomProcessorParam() const;

  [[nodiscard]] std::string getIdentifier() const;
  void setIdentifier(const std::string &identifier);

 protected:
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;

 private:
  std::shared_ptr<AudioParam> customProcessorParam_;
  std::string identifier_;
};

} // namespace audioapi