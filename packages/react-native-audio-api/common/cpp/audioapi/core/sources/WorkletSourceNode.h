#pragma once
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/core/utils/worklets/WorkletsRunner.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/utils/AudioArrayBuffer.hpp>
#include <audioapi/utils/AudioBuffer.h>
#include <jsi/jsi.h>

#include <memory>
#include <vector>

namespace audioapi {

#if RN_AUDIO_API_TEST
class WorkletSourceNode : public AudioScheduledSourceNode {
 public:
  explicit WorkletSourceNode(
      std::shared_ptr<BaseAudioContext> context,
      WorkletsRunner &&workletRunner)
      : AudioScheduledSourceNode(context) {}

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override {
    return processingBuffer;
  }
};
#else

class WorkletSourceNode : public AudioScheduledSourceNode {
 public:
  explicit WorkletSourceNode(
      const std::shared_ptr<BaseAudioContext> &context,
      WorkletsRunner &&workletRunner);

 protected:
  std::shared_ptr<AudioBuffer> processNode(
      const std::shared_ptr<AudioBuffer> &processingBuffer,
      int framesToProcess) override;

 private:
  WorkletsRunner workletRunner_;
  std::vector<std::shared_ptr<AudioArrayBuffer>> outputBuffsHandles_;
};
#endif // RN_AUDIO_API_TEST

} // namespace audioapi
