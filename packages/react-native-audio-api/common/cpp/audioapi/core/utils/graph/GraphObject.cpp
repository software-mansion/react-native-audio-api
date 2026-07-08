#include <audioapi/core/utils/graph/GraphObject.h>
#include <vector>

namespace audioapi::utils::graph {

bool GraphObject::canBeDestructed() const {
  return true;
}

bool GraphObject::isProcessable() const {
  return processableState_ != PROCESSABLE_STATE::NOT_PROCESSABLE;
}

void GraphObject::setProcessableState(PROCESSABLE_STATE state) {
  processableState_ = state;
}

const DSPAudioBuffer *GraphObject::getOutput() const {
  return nullptr;
}

AudioNode *GraphObject::asAudioNode() {
  return nullptr;
}

const AudioNode *GraphObject::asAudioNode() const {
  return nullptr;
}

void GraphObject::processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames) {
  // Default: do nothing. Subclasses override for actual processing.
  (void)inputs;
  (void)numFrames;
}

} // namespace audioapi::utils::graph
