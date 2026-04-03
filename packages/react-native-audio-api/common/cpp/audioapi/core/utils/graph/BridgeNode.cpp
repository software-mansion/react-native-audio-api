#include <audioapi/core/utils/graph/BridgeNode.h>
#include <vector>

namespace audioapi::utils::graph {

BridgeNode::BridgeNode(AudioParam *param) : param_(param) {}

bool BridgeNode::canBeDestructed() const {
  return true;
}

const DSPAudioBuffer *BridgeNode::getOutput() const {
  return nullptr;
}

AudioParam *BridgeNode::param() const {
  return param_;
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
  (void)numFrames;
}

} // namespace audioapi::utils::graph
