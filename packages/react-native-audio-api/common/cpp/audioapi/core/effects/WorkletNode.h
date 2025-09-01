#pragma once


#include <jsi/jsi.h>
#include <audioapi/core/utils/worklets/UiWorkletsRunner.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/AudioArray.h>

#include <memory>
#include <vector>

namespace audioapi {

#if RN_AUDIO_API_TEST
class WorkletNode : public AudioNode {
 public:
  explicit WorkletNode(
      BaseAudioContext *context,
      std::shared_ptr<worklets::ShareableWorklet> &worklet,
      size_t bufferLength,
      size_t inputChannelCount
  ) : AudioNode(context) {}

 protected:
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override {}
};
#else

using namespace facebook;

class WorkletNode : public AudioNode {
 public:
    explicit WorkletNode(
        BaseAudioContext *context,
        std::shared_ptr<worklets::ShareableWorklet> &worklet,
        size_t bufferLength,
        size_t inputChannelCount
    );

 protected:
  void processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;


 private:
  std::shared_ptr<UiWorkletsRunner> workletRunner_;
  std::shared_ptr<worklets::ShareableWorklet> shareableWorklet_;
  std::vector<jsi::Array> buffs_;
  size_t bufferLength_;
  size_t inputChannelCount_;
  size_t curBuffIndex_;
};

#endif

} // namespace audioapi
