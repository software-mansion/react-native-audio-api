#include <audioapi/compatibility/StableAPI.h>
#include <audioworklets/core/WorkletNode.h>

#include <memory>
#include <utility>
#include <vector>

namespace audioworklets {

WorkletNode::WorkletNode(
    const std::shared_ptr<audioapi::BaseAudioContext> &context,
    UIWorkletsRunner workletRunner,
    WorkletNodeDomain domain,
    const WorkletNodeOptions &options)
    : audioapi::AudioNode(context),
      downMixBuffer_(
          std::make_unique<audioapi::DSPAudioBuffer>(
              audioapi::RENDER_QUANTUM_SIZE,
              1,
              context->getSampleRate())),
      snapshots_(std::move(workletRunner), domain, options) {
  snapshots_.setSampleRate(context->getSampleRate());
  setProcessableState(GraphObject::PROCESSABLE_STATE::ALWAYS_PROCESSABLE);
}

void WorkletNode::processNode(int framesToProcess) {
  if (framesToProcess <= 0) {
    return;
  }

  downMixBuffer_->copy(*audioBuffer_);
  snapshots_.push(downMixBuffer_->getChannel(0)->begin(), static_cast<size_t>(framesToProcess));
}

void WorkletNode::processInputs(
    const std::vector<const audioapi::DSPAudioBuffer *> &inputs,
    int numFrames) {
  if (inputs.empty()) {
    getInputBuffer()->zero(0, static_cast<size_t>(numFrames));
    snapshots_.reset();
    return;
  }

  audioapi::AudioNode::processInputs(inputs, numFrames);
}

} // namespace audioworklets
