#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/AudioWorkletsRunner.h>
#include <audioworklets/utils/AudioChannelViews.h>

#include <cstddef>
#include <memory>

namespace audioworklets {

/**
 * An effect node that processes audio synchronously on the audio worklet runtime
 * each render quantum.
 *
 * Input samples are copied into an input `AudioChannelViews` pool, the worklet
 * callback receives separate stable `Float32Array[]` views for input and output,
 * and processed samples are copied from the output pool back to the node's audio
 * buffer.
 */
class WorkletProcessingNode : public audioapi::AudioNode {
 public:
  WorkletProcessingNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      AudioWorkletsRunner &&workletRunner);

  ~WorkletProcessingNode() override;
  DELETE_COPY_AND_MOVE(WorkletProcessingNode);

 protected:
  void processNode(int framesToProcess) override;

 private:
  AudioWorkletsRunner workletRunner_;
  std::shared_ptr<AudioChannelViews> inputChannelViews_;
  std::shared_ptr<AudioChannelViews> outputChannelViews_;
};

} // namespace audioworklets
