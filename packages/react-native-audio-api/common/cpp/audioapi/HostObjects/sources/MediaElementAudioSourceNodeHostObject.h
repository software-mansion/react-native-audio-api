#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/HostObjects/sources/MediaElementAudioSourceNodeHostObject.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/types/NodeOptions.h>
#include <memory>

namespace audioapi {

class MediaElementAudioSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit MediaElementAudioSourceNodeHostObject(
      const std::shared_ptr<AudioContext> &context,
      AudioFileSourceNode *fileSource)
      : AudioNodeHostObject(
            context->getGraph(),
            std::make_unique<MediaElementAudioSourceNode>(
                context,
                fileSource,
                MediaElementAudioSourceOptions(static_cast<int>(fileSource->getChannelCount())))) {}
};

} // namespace audioapi
