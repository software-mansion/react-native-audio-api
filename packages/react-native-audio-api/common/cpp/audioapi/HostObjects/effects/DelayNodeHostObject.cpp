#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/effects/DelayNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/effects/DelayNode.h>
#include <audioapi/core/effects/delay/DelayLine.h>
#include <audioapi/core/effects/delay/host_nodes/DelayReaderHostNode.h>
#include <audioapi/core/effects/delay/host_nodes/DelayWriterHostNode.h>
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
          options) {
  auto *delayNode = static_cast<DelayNode *>(node_->handle->audioNode->asAudioNode());
  delayTimeParam_ =
      std::make_shared<AudioParamHostObject>(graph_, node_, delayNode->getDelayTimeParam());

  auto delayBuffer = std::make_shared<AudioBuffer>(
      static_cast<size_t>(options.maxDelayTime * context->getSampleRate() + 1),
      channelCount_,
      context->getSampleRate());

  // order has to be preserved because adding cycle would not change their order in the graph
  delayReaderHostNode_ =
      std::make_shared<DelayReaderHostNode>(graph_, std::move(delayNode->delayReader_));
  delayWriterHostNode_ =
      std::make_shared<DelayWriterHostNode>(graph_, std::move(delayNode->delayWriter_));

  // Reader and writer communicate via the delay line (a ring buffer), not via
  // an audio edge. Linking their processable state ensures that when the
  // reader becomes processable (because something downstream pulls from it),
  // the writer — and everything feeding the writer — is also marked
  // processable. The same link carries the transition back to NOT_PROCESSABLE
  // on disconnect.
  audioapi::utils::graph::Graph::linkNodes(delayReaderHostNode_->rawNode(), delayWriterHostNode_->rawNode());

  addGetters(JSI_EXPORT_PROPERTY_GETTER(DelayNodeHostObject, delayTime));
}

JSI_PROPERTY_GETTER_IMPL(DelayNodeHostObject, delayTime) {
  return jsi::Object::createFromHostObject(runtime, delayTimeParam_);
}

size_t DelayNodeHostObject::getSizeInBytes() const {
  auto *delayNode = static_cast<DelayNode *>(node_->handle->audioNode->asAudioNode());
  auto base = sizeof(float) * delayNode->getDelayTimeParam()->getMaxValue();
  return static_cast<size_t>(base * delayNode->getContextSampleRate());
}

} // namespace audioapi
