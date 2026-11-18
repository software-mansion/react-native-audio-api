#include <audioapi/HostObjects/AudioConsumerHostObject.h>
#include <audioapi/HostObjects/TypedAudioNodePtr.hpp>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>
#include <audioapi/core/BaseAudioContext.h>

#include <memory>

namespace audioapi {

RecorderAdapterNodeHostObject::RecorderAdapterNodeHostObject(
    const std::shared_ptr<BaseAudioContext> &context,
    const AudioScheduledSourceNodeOptions &options)
    : AudioNodeHostObject(
          context->getGraph(),
          std::make_unique<RecorderAdapterNode>(context),
          options),
      consumer_(typedAudioNode<RecorderAdapterNode>(node_)->consumer()) {
  addGetters(JSI_EXPORT_PROPERTY_GETTER(RecorderAdapterNodeHostObject, consumer));
}

/// Each read wraps the same consumer, so the recorder sees one consumer however often JS reads it.
JSI_PROPERTY_GETTER_IMPL(RecorderAdapterNodeHostObject, consumer) {
  return jsi::Object::createFromHostObject(
      runtime, std::make_shared<AudioConsumerHostObject>(consumer_));
}

} // namespace audioapi
