#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/utils/Macros.h>
#include <cstdint>

#include <ranges>
#include <vector>

namespace audioapi {
class AudioNode;
} // namespace audioapi

namespace audioapi::utils::graph {

/// @brief Base class for graph objects (AudioNode, BridgeNode, etc.).
///
/// GraphObjects are owned by NodeHandles and stored in AudioGraph's flat vector
///
/// ## Lifecycle
/// - Created on the main thread as a unique_ptr
/// - Transferred to AudioGraph via NodeHandle on node addition
/// - Accessed on the audio thread during processing
/// - Destroyed when all below conditions are met:
///   1. The HostNode is removed and the NodeHandle is marked as a ghost
///   2. The Node has no inputs
///   3. canBeDestructed() returns true
///
/// ## Processing Model
/// - `isProcessable()` determines if the node participates in audio processing
/// - `process()` is called during graph iteration for processable nodes
/// - `getOutput()` returns the output buffer; nullptr means not mixable as input
class GraphObject {
 public:
  GraphObject() = default;
  virtual ~GraphObject() = default;
  DELETE_COPY_AND_MOVE(GraphObject);

  /// @brief Returns whether this graph object can be safely destroyed.
  [[nodiscard]] virtual bool canBeDestructed() const;

  /// @brief Controls how `isProcessable()` is derived for nodes that support conditional processing.
  enum class PROCESSABLE_STATE : std::uint8_t {
    ALWAYS_PROCESSABLE,
    NOT_PROCESSABLE,
    CONDITIONAL_PROCESSABLE,
  };

  /// @brief Returns whether this node should be processed during audio iteration.
  [[nodiscard]] virtual bool isProcessable() const;

  void setProcessableState(PROCESSABLE_STATE state);

  /// @brief Returns the output buffer for this node.
  /// @return Pointer to output buffer, or nullptr if this node should not
  ///         contribute to input mixing for other nodes.
  [[nodiscard]] virtual const DSPAudioBuffer *getOutput() const;

  /// @brief Processes this node with the given inputs.
  /// Filters inputs to only those with valid output buffers.
  /// @tparam R Range of GraphObject references (inputs from the graph)
  /// @param inputs Range of input GraphObjects
  /// @param numFrames Number of audio frames to process
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_reference_t<R>, const GraphObject &>
  void process(R &&inputs, int numFrames) {
    // Collect valid input buffers (those with non-null getOutput)
    inputBuffers_.clear();
    for (const GraphObject &input : inputs) {
      if (const DSPAudioBuffer *output = input.getOutput()) {
        inputBuffers_.push_back(output);
      }
    }
    processInputs(inputBuffers_, numFrames);
  }

  /// @brief Downcast helper for JS thread communication with AudioNode.
  [[nodiscard]] virtual AudioNode *asAudioNode();

  /// @brief Downcast helper for JS thread communication with AudioNode.
  [[nodiscard]] virtual const AudioNode *asAudioNode() const;

 protected:
  friend class HostGraph;
  /// @brief Implementation of processing logic with filtered input buffers.
  /// @param inputs Vector of pointers to valid input buffers
  /// @param numFrames Number of audio frames to process
  virtual void processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames);

  PROCESSABLE_STATE processableState_ = PROCESSABLE_STATE::NOT_PROCESSABLE;

 private:
  // Reusable buffer for collecting inputs (avoids allocation per frame)
  std::vector<const DSPAudioBuffer *> inputBuffers_;
};

} // namespace audioapi::utils::graph
