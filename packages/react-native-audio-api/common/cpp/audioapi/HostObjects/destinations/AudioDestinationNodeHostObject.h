#pragma once

#include <audioapi/HostObjects/AudioNodeHostObject.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioDestinationNodeHostObject : public AudioNodeHostObject {
 public:
  explicit AudioDestinationNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioDestinationOptions &options = AudioDestinationOptions())
      : AudioNodeHostObject(context->getGraph(),
                            std::make_unique<AudioDestinationNode>(context),
                            options) {
        context->initialize(static_cast<AudioDestinationNode*>(node_->handle->audioNode.get()));
      }
};

} // namespace audioapi
