#pragma once

#include <audioapi/core/AudioParam.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/graph/GraphObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <vector>

namespace audioapi {
class AudioParam;
}

namespace audioapi::utils::graph {

/// @brief Processable graph node that bridges AudioNode outputs to AudioParam inputs.
///
/// A BridgeNode sits between source AudioNode(s) and the owner AudioNode of a
/// param, forming the path: source(s) → bridge → owner. This lets the graph
/// system detect cycles and compute correct topological ordering for param
/// connections.
///
/// BridgeNodes are:
///   - **Processable** — mixes inputs and writes directly to AudioParam's input buffer.
///   - **Not mixable** — getOutput() returns nullptr, so other nodes won't mix it.
///   - **Always destructible** — removed by compaction when orphaned with no inputs.
///
/// ## Lifetime Management
/// - HostAudioParam owns the BridgeNode
/// - BridgeNode holds raw pointer to AudioParam (lifetime guaranteed by owner)
/// - When HostAudioParam is destructed, BridgeNode becomes orphaned
/// - canBeDestructed() always returns true, so it's cleaned up on next compaction
class BridgeNode final : public GraphObject {
 public:
  explicit BridgeNode(AudioParam *param);

  [[nodiscard]] bool canBeDestructed() const override;

  /// @brief Returns nullptr - BridgeNode should not be mixed as input for other nodes.
  [[nodiscard]] const DSPAudioBuffer *getOutput() const override;

  /// @brief Returns the param this bridge represents a connection to.
  [[nodiscard]] AudioParam *param() const;

 protected:
  void processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames) override;

 private:
  AudioParam *param_;
};

} // namespace audioapi::utils::graph
