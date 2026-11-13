#pragma once

#include <audioapi/core/AudioNode.h>
#include <memory>

namespace audioapi {

/// @brief The point where an `<Audio>` element enters the audio graph.
///
/// The element's file source is connected to this node's input like any other
/// node, so the node only forwards what the graph already mixed into its
/// buffer. It exists to give the application one stable node to connect from
/// while the element replaces its file source underneath (after every
/// playthrough, or when its `source` changes).
class MediaElementAudioSourceNode : public AudioNode {
 public:
  explicit MediaElementAudioSourceNode(const std::shared_ptr<BaseAudioContext> &context);

 protected:
  void processNode(int framesToProcess) override;
};

} // namespace audioapi
