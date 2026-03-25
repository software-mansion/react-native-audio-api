#include <audioapi/HostObjects/destinations/AudioDestinationNodeHostObject.h>

#include <memory>
#include <utility>

namespace audioapi {

AudioDestinationNodeHostObject::AudioDestinationNodeHostObject(
    utils::graph::HostGraph::Node *node,
    std::shared_ptr<AudioDestinationNode> destination)
    : node_(node), destination_(std::move(destination)) {
  addGetters(
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, numberOfInputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, numberOfOutputs),
      JSI_EXPORT_PROPERTY_GETTER(AudioDestinationNodeHostObject, channelCount));
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, numberOfInputs) {
  return {1};
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, numberOfOutputs) {
  return {0};
}

JSI_PROPERTY_GETTER_IMPL(AudioDestinationNodeHostObject, channelCount) {
  return {static_cast<int>(destination_->getChannelCount())};
}

} // namespace audioapi
