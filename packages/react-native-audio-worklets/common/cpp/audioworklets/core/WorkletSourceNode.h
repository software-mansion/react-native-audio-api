#pragma once

#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/AudioWorkletsRunner.h>
#include <audioworklets/utils/AudioChannelViews.h>

#include <memory>

namespace audioworklets {

/**
 * A scheduled source node that generates audio synchronously on the audio worklet
 * runtime each render quantum.
 *
 * The worklet callback receives stable `Float32Array[]` views over an output pool;
 * generated samples are copied into the node's audio buffer at the scheduled offset.
 */
class WorkletSourceNode : public audioapi::AudioScheduledSourceNode {
 public:
  WorkletSourceNode(
      const std::shared_ptr<audioapi::BaseAudioContext> &context,
      AudioWorkletsRunner &&workletRunner);

  ~WorkletSourceNode() override;
  DELETE_COPY_AND_MOVE(WorkletSourceNode);

 protected:
  void processNode(int framesToProcess) override;

 private:
  AudioWorkletsRunner workletRunner_;
  std::shared_ptr<AudioChannelViews> outputChannelViews_;
};

} // namespace audioworklets
