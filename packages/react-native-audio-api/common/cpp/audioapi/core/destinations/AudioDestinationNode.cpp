#include <audioapi/core/AudioNode.h>
#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

AudioDestinationNode::AudioDestinationNode(const std::shared_ptr<BaseAudioContext> &context)
    : AudioNode(context, AudioDestinationOptions()) {
  isInitialized_.store(true, std::memory_order_release);
}

} // namespace audioapi
