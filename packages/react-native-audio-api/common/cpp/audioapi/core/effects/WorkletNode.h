#pragma once


#include <jsi/jsi.h>
#include <audioapi/core/utils/worklets/WorkletsRunner.h>
#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/utils/AudioBus.h>
#include <audioapi/utils/AudioArray.h>
#include <audioapi/jsi/AudioArrayBuffer.h>

#include <memory>
#include <vector>

namespace audioapi {

#if RN_AUDIO_API_TEST
class WorkletNode : public AudioNode {
 public:
  explicit WorkletNode(
      BaseAudioContext *context,
      size_t bufferLength,
      size_t inputChannelCount,
      WorkletsRunner &&workletRunner
  ) : AudioNode(context) {}

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override { return processingBus; }
};
#else

using namespace facebook;

class WorkletNode : public AudioNode {
 public:
  explicit WorkletNode(
      BaseAudioContext *context,
      size_t bufferLength,
      size_t inputChannelCount,
      WorkletsRunner &&workletRunner
  );

  ~WorkletNode() override = default;

 protected:
  std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus>& processingBus, int framesToProcess) override;


 private:
  WorkletsRunner workletRunner_;
  std::shared_ptr<AudioBus> bus_;

  /// @brief Length of the byte buffer that will be passed to the AudioArrayBuffer
  size_t bufferLength_;
  size_t inputChannelCount_;
  size_t curBuffIndex_;
};

#endif // RN_AUDIO_API_TEST

} // namespace audioapi
