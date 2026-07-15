#pragma once

#include <audioapi/compatibility/StableAPI.h>

namespace audioapi {

class MyProcessorNode : public AudioNode {
 public:
  explicit MyProcessorNode(const std::shared_ptr<BaseAudioContext> &context);

 protected:
  void processNode(int framesToProcess) override;
};

} // namespace audioapi
