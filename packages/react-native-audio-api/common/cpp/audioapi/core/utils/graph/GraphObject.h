#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <audioapi/utils/Macros.h>
#include <cstddef>
#include <cstdint>

#include <ranges>
#include <utility>
#include <vector>

namespace audioapi {
class AudioNode;
} // namespace audioapi

namespace audioapi::utils::graph {

class AudioGraph;

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

  /// @brief Called once per render quantum, instead of `process()`, for every node that
  /// is not processable in that quantum. `currentSampleFrame` is the first frame of the
  /// quantum being rendered. A node that is not pulled by any consumer never runs its
  /// render step, so this is its only chance to observe the context clock. Scheduled
  /// sources use it to reach their finished state (and fire `ended`) once their stop time
  /// has passed while disconnected; otherwise they would never become destructible.
  /// Default: no-op. Must stay allocation-free.
  /// @note Audio thread only
  virtual void advanceWithoutRender(std::size_t currentSampleFrame);

  void setProcessableState(PROCESSABLE_STATE state);

  /// @brief Returns the output buffer for this node.
  /// @return Pointer to output buffer, or nullptr if this node should not
  ///         contribute to input mixing for other nodes.
  [[nodiscard]] virtual const DSPAudioBuffer *getOutput() const;

  /// @brief Whether a cycle of audio edges may run through this node.
  ///
  /// The graph accepts a cycle only when it contains such a node.
  /// While the node breaks a cycle it is rendered in two steps:
  /// `process()` runs `processReader()` at a position that ignores its
  /// inputs, and `processAfterGraph()` hands it those inputs once every node
  /// has rendered. Its output is therefore built from inputs of earlier quanta.
  [[nodiscard]] virtual bool canBreakCycle() const;

  /// @brief Processes this node with the given inputs.
  /// Filters inputs to only those with valid output buffers.
  /// @tparam R Range of GraphObject references (inputs from the graph)
  /// @param inputs Range of input GraphObjects
  /// @param numFrames Number of audio frames to process
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_reference_t<R>, const GraphObject &>
  void process(R &&inputs, int numFrames) {
    if (breaksCycle_) {
      processReader(numFrames);
      return;
    }
    collectInputBuffers(inputs);
    processInputs(inputBuffers_, numFrames);
    returnToIdleAfterPull();
  }

  /// @brief Runs `processWriter()` with `inputs` (see `canBreakCycle()`).
  /// Call after every node of the graph has rendered this quantum.
  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_reference_t<R>, const GraphObject &>
  void processAfterGraph(R &&inputs, int numFrames) {
    collectInputBuffers(inputs);
    processWriter(inputBuffers_, numFrames);
    returnToIdleAfterPull();
  }

  /// @brief Swaps in a pre-reserved input-scratch vector and returns the
  /// previous one.
  ///
  /// `inputBuffers_` is audio-thread-only scratch reused by `process()`. To keep
  /// `process()` allocation-free without ever mutating it from the JS thread,
  /// `HostGraph::addEdge` reserves a replacement vector off the audio thread and
  /// hands it here from the graph-mutation event. The returned old vector is
  /// then disposed off the audio thread so no `free()` happens during rendering.
  /// @note Audio thread only.
  std::vector<const DSPAudioBuffer *> exchangeInputScratch(
      std::vector<const DSPAudioBuffer *> reserved) {
    std::swap(inputBuffers_, reserved);
    return reserved;
  }

  /// @brief Downcast helper for JS thread communication with AudioNode.
  [[nodiscard]] virtual AudioNode *asAudioNode();

  /// @brief Downcast helper for JS thread communication with AudioNode.
  [[nodiscard]] virtual const AudioNode *asAudioNode() const;

 protected:
  friend class HostGraph;
  friend class AudioGraph;
  /// @brief Implementation of processing logic with filtered input buffers.
  /// @param inputs Vector of pointers to valid input buffers
  /// @param numFrames Number of audio frames to process
  virtual void processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames);

  /// @brief The reader half of a node that breaks a cycle (the spec's DelayReader):
  /// renders this quantum's output before the node's inputs have rendered.
  virtual void processReader(int numFrames);

  /// @brief The writer half (the spec's DelayWriter): takes the inputs of the
  /// quantum whose output `processReader()` already rendered. Must not write to
  /// the output buffer: another node on the cycle may still read it.
  virtual void processWriter(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames);

  /// @brief Whether this node breaks a cycle right now: it lies on one and
  /// `canBreakCycle()` is true. The other nodes of that cycle never have it set.
  /// @note Audio Thread only
  bool breaksCycle_ = false;

  PROCESSABLE_STATE processableState_ = PROCESSABLE_STATE::NOT_PROCESSABLE;

  /// @brief When set, AudioGraph::settleProcessableState() will never promote
  /// this node back to CONDITIONAL_PROCESSABLE during the dependency pull.
  ///
  /// Used to make `disable()` sticky: a source that finished playback while
  /// still connected to a processable downstream must stay idle for good,
  /// otherwise the every-quantum reverse pull would re-activate it (it is
  /// still an input of a processable consumer).
  /// @note Audio Thread only
  bool excludeFromProcessablePull_ = false;

 private:
  /// @brief Fills `inputBuffers_` with the outputs of the inputs that render
  /// this quantum and have an output buffer.
  template <std::ranges::input_range R>
  void collectInputBuffers(R &&inputs) {
    inputBuffers_.clear();
    for (const GraphObject &input : inputs) {
      if (!input.rendersThisQuantum_) {
        continue;
      }
      if (const DSPAudioBuffer *output = input.getOutput()) {
        inputBuffers_.push_back(output);
      }
    }
  }

  void returnToIdleAfterPull() {
    // Conditional nodes are pulled for one quantum only
    if (processableState_ == PROCESSABLE_STATE::CONDITIONAL_PROCESSABLE) {
      processableState_ = PROCESSABLE_STATE::NOT_PROCESSABLE;
    }
  }

  // Reusable buffer for collecting inputs (avoids allocation per frame)
  std::vector<const DSPAudioBuffer *> inputBuffers_;

  /// @brief Whether this node is processable in the quantum being rendered, set by
  /// AudioGraph::advanceUnpulledNodes(). A node left out keeps its last output in
  /// its buffer, so consumers must skip it. Otherwise a node that keeps rendering
  /// its tail after a disconnect would mix that stale output forever and its tail
  /// would never end.
  /// @note Audio Thread only
  bool rendersThisQuantum_ = true;
};

} // namespace audioapi::utils::graph
