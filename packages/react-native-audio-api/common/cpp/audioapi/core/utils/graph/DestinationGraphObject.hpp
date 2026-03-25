#pragma once

#include <audioapi/core/destinations/AudioDestinationNode.h>
#include <audioapi/core/utils/graph/GraphObject.hpp>

namespace audioapi {

class DestinationGraphObject final : public utils::graph::GraphObject {
 public:
  explicit DestinationGraphObject(AudioDestinationNode *destination) : destination_(destination) {}

  AudioNode *asAudioNode() override {
    return destination_;
  }

  const AudioNode *asAudioNode() const override {
    return destination_;
  }

  // Context never removes the destination from the graph.
  bool canBeDestructed() const override {
    return false;
  }

 private:
  AudioDestinationNode *destination_; // non-owning; lifetime guaranteed by BaseAudioContext
};

} // namespace audioapi
