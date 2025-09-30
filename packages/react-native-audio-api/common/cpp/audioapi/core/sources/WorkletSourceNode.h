#pragma once
#include <jsi/jsi.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/core/sources/AudioScheduledSourceNode.h>
#include <audioapi/core/utils/worklets/SafeIncludes.h>
#include <audioapi/core/utils/worklets/WorkletsRunner.h>
#include <audioapi/jsi/AudioArrayBuffer.h>
#include <audioapi/utils/AudioArray.h>

#include <vector>
#include <memory>

namespace audioapi {

class WorkletSourceNode : public AudioScheduledSourceNode {
 public:
  explicit WorkletSourceNode(
    BaseAudioContext *context,
    std::shared_ptr<worklets::SerializableWorklet> &worklet,
    std::weak_ptr<worklets::WorkletRuntime> runtime
  );

 protected:
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;
 private:
  WorkletsRunner workletRunner_;
  std::shared_ptr<worklets::SerializableWorklet> shareableWorklet_;
  std::vector<std::shared_ptr<AudioArrayBuffer>> outputBuffsHandles_;
};

} // namespace audioapi
