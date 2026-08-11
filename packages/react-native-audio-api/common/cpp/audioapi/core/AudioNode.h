#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/GraphObject.h>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

class AudioParam;

class AudioNode : public utils::graph::GraphObject, public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioNodeOptions &options = AudioNodeOptions());
  ~AudioNode() override = default;
  DELETE_COPY_AND_MOVE(AudioNode);

  /// @brief Returns this node's `channelCount` attribute.
  /// @note Safe to call from any thread: `channelCount_` is atomic because
  /// source subclasses update it on the audio thread while the JS thread reads
  /// it during channel-count negotiation.
  [[nodiscard]] size_t getChannelCount() const;

  /// @brief Returns this node's `channelCountMode` attribute.
  /// @note Read only on the host thread (channel-count negotiation) — never on
  /// the audio thread — so mutating it from the JS thread via
  /// `setChannelCountMode` is race-free with audio processing.
  [[nodiscard]] ChannelCountMode getChannelCountMode() const {
    return channelCountMode_;
  }

  [[nodiscard]] ChannelInterpretation getChannelInterpretation() const {
    return channelInterpretation_;
  }

  /// @brief Sets `channelCount`. Drives channel-count negotiation, which reads
  /// this value on the host thread. Callers must trigger a renegotiation so
  /// the change propagates to buffer layouts.
  /// @note Host (JS) thread only. Overridable for node-specific constraints.
  virtual void setChannelCount(size_t channelCount) {
    channelCount_ = static_cast<int>(channelCount);
  }

  /// @brief Sets `channelCountMode`. Read only on the host thread during
  /// negotiation. Callers must trigger a renegotiation afterwards.
  /// @note Host (JS) thread only. Overridable for node-specific constraints.
  virtual void setChannelCountMode(ChannelCountMode channelCountMode) {
    channelCountMode_ = channelCountMode;
  }

  /// @brief Sets `channelInterpretation`. This value is read on the audio
  /// thread in `processInputs` (up/down-mix summing), so it MUST be applied via
  /// a scheduled audio event rather than mutated directly from the JS thread.
  /// @note Audio thread only. Overridable for node-specific constraints.
  virtual void setChannelInterpretation(ChannelInterpretation channelInterpretation) {
    channelInterpretation_ = channelInterpretation;
  }

  [[nodiscard]] float getContextSampleRate() const {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->getSampleRate();
    }

    return DEFAULT_SAMPLE_RATE;
  }

  [[nodiscard]] float getNyquistFrequency() const {
    constexpr float kNyquistDivisor = 2.0f;
    return getContextSampleRate() / kNyquistDivisor;
  }

  /// @brief Returns the output buffer for this node.
  /// @note Audio Thread only.
  [[nodiscard]] const DSPAudioBuffer *getOutput() const override {
    return audioBuffer_.get();
  }

  /// @brief Returns the input buffer for this node. By default, this is the same as the output buffer.
  /// @note Audio Thread only.
  /// @note For StereoPannerNode and PannerNode due to channel limitations -
  /// https://webaudio.github.io/web-audio-api/#StereoPanner-channel-limitations
  /// the input buffer is negotiate with inputs, but output buffer is always stereo.
  std::shared_ptr<DSPAudioBuffer> getInputBuffer() const {
    return audioBuffer_;
  }

  /// @brief Returns the output buffer for this node. By default, this is the same as the input buffer.
  /// @note Audio Thread only.
  /// @note For StereoPannerNode and PannerNode due to channel limitations -
  /// https://webaudio.github.io/web-audio-api/#StereoPanner-channel-limitations
  /// the input buffer is negotiate with inputs, but output buffer is always stereo.
  virtual std::shared_ptr<DSPAudioBuffer> getOutputBuffer() const {
    return audioBuffer_;
  }

  virtual void setOutputBuffer(const std::shared_ptr<DSPAudioBuffer> &buffer) {
    audioBuffer_ = buffer;
  }

  /// @brief Buffer whose channel layout is negotiated from upstream connections.
  /// Default: same as the output buffer. StereoPanner negotiates the input
  /// buffer while keeping a fixed stereo output.
  [[nodiscard]] virtual std::shared_ptr<DSPAudioBuffer> getNegotiatedBuffer() const {
    return getOutputBuffer();
  }

  virtual void setNegotiatedBuffer(const std::shared_ptr<DSPAudioBuffer> &buffer) {
    setOutputBuffer(buffer);
  }

  /// @brief Channel count this node presents on upstream connections (toward
  /// AudioDestinationNode) after negotiation. Default: the negotiated channel
  /// count. StereoPanner always outputs stereo.
  [[nodiscard]] virtual size_t getUpstreamChannelCount(size_t negotiatedChannelCount) const {
    return negotiatedChannelCount;
  }

  /// @note JS Thread only
  [[nodiscard]] bool requiresTailProcessing() const;

  /// @brief Tail-processing lifecycle state.
  ///
  /// Drives the "node has a non-zero impulse response that
  /// must be played out after its inputs go silent" behavior required by the
  /// Web Audio spec for nodes such as BiquadFilter, Delay and Convolver.
  ///
  /// Initial state is `FINISHED` — a fresh node has no in-flight tail to
  /// render; non-silent input re-arms it to `ACTIVE`.
  ///
  /// Transitions (in `processInputs`, gated by `requiresTailProcessing_`):
  /// - any quantum with non-silent input        : * -> ACTIVE (re-arm)
  /// - first silent quantum after ACTIVE        : ACTIVE -> SIGNALLED_TO_STOP,
  ///                                              tailFramesRemaining_ = computeTailFrames()
  /// - subsequent silent quanta                 : tailFramesRemaining_ -= numFrames
  ///                                              once <= 0, SIGNALLED_TO_STOP -> FINISHED
  /// - silent quantum while FINISHED            : stay FINISHED
  ///
  /// @note Audio Thread only
  enum class TailState : std::uint8_t {
    ACTIVE,
    SIGNALLED_TO_STOP,
    FINISHED,
  };

  /// @brief Returns whether this node should be processed during audio iteration.
  ///
  /// Extends the base GraphObject rule with: a tail-bearing node remains
  /// processable until its tail has fully drained, even after its
  /// `processableState_` was flipped to NOT_PROCESSABLE by a downstream
  /// disconnect.
  /// @note Audio Thread only.
  [[nodiscard]] bool isProcessable() const override;

  template <typename F>
  bool scheduleAudioEvent(F &&event) noexcept {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->scheduleAudioEvent(std::forward<F>(event));
    }

    return false;
  }

  /// @brief Schedule an audio event from the JS runtime's finalizer (GC)
  /// thread. Use this from `~HostObject` code paths — `scheduleAudioEvent`
  /// would race because its SPSC channel's single producer is the JS
  /// thread.
  template <typename F>
  bool scheduleGCEvent(F &&event) noexcept {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->scheduleGCEvent(std::forward<F>(event));
    }

    return false;
  }

  bool canBeDestructed() const override;

  [[nodiscard]] AudioNode *asAudioNode() override {
    return this;
  }

  [[nodiscard]] const AudioNode *asAudioNode() const override {
    return this;
  }

 protected:
  friend class DelayNodeHostObject;
  friend class utils::graph::HostGraph;

  std::weak_ptr<BaseAudioContext> context_;
  std::shared_ptr<DSPAudioBuffer> audioBuffer_;

  const int numberOfInputs_ = 1;
  const int numberOfOutputs_ = 1;
  /// @brief Number of channels this node presents.
  ///
  /// Atomic because it is read on the JS thread during channel-count
  /// negotiation (`HostGraph`/`getChannelCount`) while source subclasses
  /// (AudioBufferSource, Streamer, AudioFileSource, RecorderAdapter,
  /// AudioBufferQueueSource) write it on the audio thread once they learn the
  /// decoded/buffer channel count. Plain reads/writes here would race.
  std::atomic<int> channelCount_ = 2;
  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ = ChannelInterpretation::SPEAKERS;
  const bool requiresTailProcessing_;

  /// @brief Tail-processing audio-thread state. Only mutated when
  /// `requiresTailProcessing_` is true.
  TailState tailState_ = TailState::FINISHED;
  int tailFramesRemaining_ = 0;

  /// @brief Set by `disable()` when a source finishes, cleared on the next
  /// `processInputs`. Defers flipping the node to NOT_PROCESSABLE by one
  /// quantum so the final output produced this quantum is still mixed by
  /// downstream consumers (which gate on `isProcessable()`).
  /// @note Audio Thread only
  bool pendingDisable_ = false;

  /// @brief Implementation of processing logic for AudioNode.
  /// Mixes input buffers, runs the tail-state transition (when this node
  /// requires tail processing), and calls processNode.
  void processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames) override {
    // Deferred disable: a source that finished during the PREVIOUS quantum
    // requested a disable but stayed processable so its final output could be
    // mixed by downstream consumers that same quantum. Commit the flip now, at
    // the start of the next quantum, present silence, and stop processing.
    // Consumers gate mixing on isProcessable(), so from here on this node
    // contributes silence. See AudioNode::disable().
    if (pendingDisable_) {
      pendingDisable_ = false;
      // do not mark this node as processable in the next quantum
      excludeFromProcessablePull_ = true;
      getOutputBuffer()->zero();
      return;
    }

    if (requiresTailProcessing_) {
      updateTailStateForQuantum(inputs, numFrames);
    }

    getInputBuffer()->zero();

    for (const DSPAudioBuffer *input : inputs) {
      getInputBuffer()->sum(*input, channelInterpretation_);
    }

    processNode(numFrames);
  }

  /// @brief Returns the tail length in audio frames for the current node
  /// state. Called when transitioning into `SIGNALLED_TO_STOP` so that the
  /// frame counter starts from a fresh, up-to-date value.
  ///
  /// Default 0 — overriders are tail-bearing nodes (Biquad, Convolver,
  /// DelayWriter). The implementation may read audio-thread-owned state
  /// (filter coefficients, IR length, max delay).
  ///
  /// @note Audio Thread only.
  [[nodiscard]] virtual int computeTailFrames() const {
    return 0;
  }

  /// @brief Returns whether the set of input buffers carries no signal this
  /// quantum. Default scans every sample. Tail-bearing nodes whose audio
  /// inputs come from outside the graph (none today, but plausible) may
  /// override.
  ///
  /// @note Audio Thread only.
  [[nodiscard]] virtual bool isInputSilent(const std::vector<const DSPAudioBuffer *> &inputs) const;

  /// @brief Requests that this node stop participating in graph processing.
  /// Sets `alwaysNotProcessable_` so settle will not re-activate it;
  /// the current quantum's output remains available for downstream mixing,
  /// and the next settle zeros the idle buffer.
  /// @note Audio Thread only. Source nodes call this when playback finishes.
  virtual void disable();

  virtual void processNode(int) = 0;

 private:
  /// @brief Drives `tailState_` for one render quantum. Called from
  /// `processInputs` only when `requiresTailProcessing_` is true.
  void updateTailStateForQuantum(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames);
};

} // namespace audioapi
