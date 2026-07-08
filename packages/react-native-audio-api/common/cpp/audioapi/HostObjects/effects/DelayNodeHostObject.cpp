#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.h>
#include <audioapi/HostObjects/effects/DelayNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/effects/delay/DelayLine.h>
#include <audioapi/core/effects/delay/host_nodes/DelayReaderHostNode.h>
#include <audioapi/core/effects/delay/host_nodes/DelayWriterHostNode.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>
#include <utility>

namespace audioapi {

DelayNodeHostObject::DelayNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const DelayOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<DelayNode>(context, options),
          options),
      delayNode_(typedAudioNode<DelayNode>(node_)) {
  delayTimeParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, delayNode_->getDelayTimeParam());

  auto delayBuffer = std::make_shared<AudioBuffer>(
      static_cast<size_t>(options.maxDelayTime * context->getSampleRate() + 1),
      channelCount_,
      context->getSampleRate());

  // order has to be preserved because adding cycle would not change their order in the graph
  delayReaderHostNode_ =
      std::make_shared<DelayReaderHostNode>(graph_, std::move(delayNode_->delayReader_));
  delayWriterHostNode_ =
      std::make_shared<DelayWriterHostNode>(graph_, std::move(delayNode_->delayWriter_));

  // Reader and writer communicate via the delay line (a ring buffer), not via
  // an audio edge. Linking their processable state ensures that when the
  // reader becomes processable (because something downstream pulls from it),
  // the writer — and everything feeding the writer — is also marked
  // processable. The same link carries the transition back to NOT_PROCESSABLE
  // on disconnect.
  audioapi::utils::graph::Graph::linkNodes(
      delayReaderHostNode_->rawNode(), delayWriterHostNode_->rawNode());

  addGetters(JSI_EXPORT_PROPERTY_GETTER(DelayNodeHostObject, delayTime));
}

JSI_PROPERTY_GETTER_IMPL(DelayNodeHostObject, delayTime) {
  return jsi::Object::createFromHostObject(runtime, delayTimeParam_);
}

size_t DelayNodeHostObject::getMemoryPressure() const {
  const float maxDelaySeconds = delayNode_->getDelayTimeParam()->getMaxValue();
  const float sampleRate = delayNode_->getContextSampleRate();
  // The delay line ring buffer dominates: (maxDelay * sr + 1) frames * channels * float.
  const size_t ringBytes =
      static_cast<size_t>(maxDelaySeconds * sampleRate + 1) * channelCount_ * sizeof(float);
  // Base `audioBuffer_` from AudioNodeHostObject::getMemoryPressure(), plus
  // the reader/writer AudioNode sub-nodes (each owns its own RQ audioBuffer_)
  // and the delayTime AudioParam.
  return AudioNodeHostObject::getMemoryPressure() +
      2 * RENDER_QUANTUM_SIZE * channelCount_ * sizeof(float) + kAudioParamBytes + ringBytes;
}

} // namespace audioapi
