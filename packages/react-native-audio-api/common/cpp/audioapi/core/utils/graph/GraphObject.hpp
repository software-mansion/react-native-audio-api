#pragma once

namespace audioapi {
class AudioNode;
class AudioParam;
} // namespace audioapi

namespace audioapi::utils::graph {

/// @brief Base class for graph objects (AudioNode or AudioParam).
/// GraphObjects are owned by NodeHandles and stored in AudioGraph's flat vector
///
/// ## Lifecycle
/// - Created on the main thread as a unique_ptr
/// - Transferred to AudioGraph via NodeHandle on node addition
/// - Accessed on the audio thread during processing (e.g. for processAudio)
/// - Destroyed when all below conditions are met:
///   1. The HostNode is removed and the NodeHandle is marked as a ghost
///   2. The Node has no inputs
///   3. canBeDestructed() returns true (e.g. AudioNode-specific lifecycle checks)
class GraphObject {
 public:
  virtual ~GraphObject() = default;

  /// @brief Returns whether this graph object can be safely destroyed.
  ///
  /// Default behavior is permissive for new GraphObject-based entities.
  /// AudioNode / AudioParam can override with richer lifecycle checks.
  [[nodiscard]] virtual bool canBeDestructed() const {
    return true;
  }

  /// @brief Downcast helper for node-specific handling.
  [[nodiscard]] virtual AudioNode *asAudioNode() {
    return nullptr;
  }

  /// @brief Downcast helper for node-specific handling.
  [[nodiscard]] virtual const AudioNode *asAudioNode() const {
    return nullptr;
  }

  /// @brief Downcast helper for param-specific handling.
  [[nodiscard]] virtual AudioParam *asAudioParam() {
    return nullptr;
  }

  /// @brief Downcast helper for param-specific handling.
  [[nodiscard]] virtual const AudioParam *asAudioParam() const {
    return nullptr;
  }
};

} // namespace audioapi::utils::graph
