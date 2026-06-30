#include <audioapi/core/AudioNode.h>
#include <audioapi/core/AudioParam.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioArray.hpp>

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace audioapi {

AudioNode::AudioNode(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioNodeOptions &options)
    : context_(context),
      numberOfInputs_(options.numberOfInputs),
      numberOfOutputs_(options.numberOfOutputs),
      channelCount_(options.channelCount),
      channelCountMode_(options.channelCountMode),
      channelInterpretation_(options.channelInterpretation),
      requiresTailProcessing_(options.requiresTailProcessing) {
  audioBuffer_ = std::make_shared<DSPAudioBuffer>(
      RENDER_QUANTUM_SIZE, channelCount_, context->getSampleRate());
}

bool AudioNode::canBeDestructed() const {
  // Tail-bearing nodes must not be destroyed mid-tail: even if the node is
  // orphaned and has no live inputs, audio that has not yet been rendered
  // would otherwise be lost. Once the tail has fully drained, the node
  // becomes destructible.
  return !requiresTailProcessing_ || tailState_ == TailState::FINISHED;
}

bool AudioNode::isProcessable() const {
  if (GraphObject::isProcessable()) {
    return true;
  }
  // Even after a downstream disconnect flips processableState_ to
  // NOT_PROCESSABLE, a tail-bearing node must keep being scheduled until its
  // impulse response has decayed. Otherwise the tail would never play out.
  return requiresTailProcessing_ && tailState_ != TailState::FINISHED;
}

size_t AudioNode::getChannelCount() const {
  return channelCount_;
}

bool AudioNode::requiresTailProcessing() const {
  return requiresTailProcessing_;
}

void AudioNode::disable() {
  setProcessableState(utils::graph::GraphObject::PROCESSABLE_STATE::NOT_PROCESSABLE);
}

namespace {

// Branch-free silence check over a contiguous float span. Reinterprets each
// sample as its IEEE-754 bit pattern and ORs them together with the sign bit
// masked out, so `-0.0f` is treated as silent (matching `== 0.0f`). The lack
// of an in-loop branch lets the compiler auto-vectorize this tight loop.
[[nodiscard]] inline bool spanIsSilent(const float *data, size_t count) noexcept {
  constexpr std::uint32_t kSignMask = 0x7FFFFFFFu;
  std::uint32_t acc = 0;
  for (size_t i = 0; i < count; ++i) {
    std::uint32_t bits;
    std::memcpy(&bits, data + i, sizeof(bits));
    acc |= (bits & kSignMask);
  }
  return acc == 0;
}

} // namespace

bool AudioNode::isInputSilent(const std::vector<const DSPAudioBuffer *> &inputs) const {
  // No inputs means upstream is gone (or this node never had any). Either
  // way the mixer below would feed silence to processNode, so treat it as
  // silent input here.
  if (inputs.empty()) {
    return true;
  }
  for (const DSPAudioBuffer *input : inputs) {
    if (input == nullptr) {
      continue;
    }
    const size_t channels = input->getNumberOfChannels();
    for (size_t c = 0; c < channels; ++c) {
      auto *channel = input->getChannel(c);
      if (channel == nullptr) {
        continue;
      }
      const auto span = channel->span();
      if (!spanIsSilent(span.data(), span.size())) {
        return false;
      }
    }
  }
  return true;
}

void AudioNode::updateTailStateForQuantum(
    const std::vector<const DSPAudioBuffer *> &inputs,
    int numFrames) {
  const bool silent = isInputSilent(inputs);

  if (!silent) {
    // Any non-silent sample re-arms the tail: the node is being driven again
    // and we must not stop it mid-stream.
    tailState_ = TailState::ACTIVE;
    tailFramesRemaining_ = 0;
    return;
  }

  switch (tailState_) {
    case TailState::ACTIVE:
      tailState_ = TailState::SIGNALLED_TO_STOP;
      tailFramesRemaining_ = computeTailFrames();
      if (tailFramesRemaining_ <= 0) {
        tailState_ = TailState::FINISHED;
      }
      break;
    case TailState::SIGNALLED_TO_STOP:
      tailFramesRemaining_ -= numFrames;
      if (tailFramesRemaining_ <= 0) {
        tailState_ = TailState::FINISHED;
      }
      break;
    case TailState::FINISHED:
      // Stay finished; will be reset to ACTIVE if non-silent input returns.
      break;
  }
}

} // namespace audioapi
