#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/sources/MediaElementAudioSourceNode.h>
#include <audioapi/types/NodeOptions.h>

#include <memory>

namespace audioapi {

MediaElementAudioSourceNode::MediaElementAudioSourceNode(
    const std::shared_ptr<BaseAudioContext> &context)
    : AudioNode(context, AudioNodeOptions()) {}

// simple bypass
void MediaElementAudioSourceNode::processNode(int /*framesToProcess*/) {}

} // namespace audioapi
