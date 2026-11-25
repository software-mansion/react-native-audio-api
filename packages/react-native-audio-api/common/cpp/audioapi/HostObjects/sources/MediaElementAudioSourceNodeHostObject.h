#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/AudioContext.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <memory>

namespace audioapi {

class MediaElementAudioSourceNodeHostObject : public AudioNodeHostObject {
 public:
  explicit MediaElementAudioSourceNodeHostObject(const std::shared_ptr<AudioContext> &context)
      : AudioNodeHostObject(
            context->getGraph(),
            std::make_unique<MediaElementAudioSourceNode>(context)) {}
};

} // namespace audioapi
