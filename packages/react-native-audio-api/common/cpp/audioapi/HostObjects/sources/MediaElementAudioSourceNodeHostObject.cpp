#include <audioapi/HostObjects/sources/MediaElementAudioSourceNodeHostObject.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

MediaElementAudioSourceNodeHostObject::MediaElementAudioSourceNodeHostObject(
    const std::shared_ptr<MediaElementAudioSourceNode> &node)
    : AudioNodeHostObject(
          node,
          MediaElementAudioSourceOptions(static_cast<int>(node->getChannelCount()))) {}

MediaElementAudioSourceNodeHostObject::~MediaElementAudioSourceNodeHostObject() = default;

} // namespace audioapi
