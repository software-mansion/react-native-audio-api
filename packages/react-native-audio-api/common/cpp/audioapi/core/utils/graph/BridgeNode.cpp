#include <audioapi/core/AudioParam.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/graph/BridgeNode.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <vector>

namespace audioapi::utils::graph {

void BridgeNode::beforeDestruction() {
  // Notify AudioParam that it should no longer rely on bridge input
  if (param_ != nullptr) {
    param_->onBridgeDetached();
  }
}

void BridgeNode::processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames) {
  // Skip processing if param is null (e.g., in tests)
  if (param_ == nullptr) {
    return;
  }

  // Get AudioParam's input buffer and fill it with mixed inputs
  auto inputBuffer = param_->getInputBuffer();
  inputBuffer->zero();

  for (const DSPAudioBuffer *input : inputs) {
    inputBuffer->sum(*input, ChannelInterpretation::SPEAKERS);
  }
}

} // namespace audioapi::utils::graph
