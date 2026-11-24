#pragma once

#include <audioapi/core/AudioNode.h>
#include <memory>

namespace audioapi {

/// @brief Represents an audio source from an `<Audio>` element.
///
/// The element's file source is connected to this node's input like any other
/// node, so the node only forwards what the graph already mixed into its
/// buffer.
class MediaElementAudioSourceNode : public AudioNode {
 public:
  explicit MediaElementAudioSourceNode(const std::shared_ptr<BaseAudioContext> &context);

 protected:
  void processNode(int framesToProcess) override;
};

} // namespace audioapi
