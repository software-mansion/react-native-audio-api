#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/sources/MediaElementAudioSourceNodeHostObject.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>

namespace audioapi {

class MediaElementAudioSourceNode;

class MediaElementAudioSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit MediaElementAudioSourceNodeHostObject(
      const std::shared_ptr<MediaElementAudioSourceNode> &node)
      : AudioNodeHostObject(
            node,
            MediaElementAudioSourceOptions(static_cast<int>(node->getChannelCount()))) {}
};

} // namespace audioapi
