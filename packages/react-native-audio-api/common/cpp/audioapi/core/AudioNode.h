#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/GraphObject.hpp>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

class AudioParam;

class AudioNode : public utils::graph::GraphObject, public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(const std::shared_ptr<BaseAudioContext> &context,
                     const AudioNodeOptions &options = AudioNodeOptions());
  ~AudioNode() override = default;
  DELETE_COPY_AND_MOVE(AudioNode);

  [[nodiscard]] size_t getChannelCount() const;

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

  /// @note JS Thread only
  [[nodiscard]] bool requiresTailProcessing() const;

  template <typename F>
  bool scheduleAudioEvent(F &&event) noexcept {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->scheduleAudioEvent(std::forward<F>(event));
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

  std::weak_ptr<BaseAudioContext> context_;
  std::shared_ptr<DSPAudioBuffer> audioBuffer_;

  const int numberOfInputs_ = 1;
  const int numberOfOutputs_ = 1;
  int channelCount_ = 2;
  const ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  const ChannelInterpretation channelInterpretation_ = ChannelInterpretation::SPEAKERS;
  const bool requiresTailProcessing_;

  /// @brief Implementation of processing logic for AudioNode.
  /// Mixes input buffers and calls processNode.
  void processInputs(const std::vector<const DSPAudioBuffer *> &inputs, int numFrames) override {
    getInputBuffer()->zero();

    for (const DSPAudioBuffer *input : inputs) {
      getInputBuffer()->sum(*input, channelInterpretation_);
    }

    processNode(numFrames);
  }

  virtual void processNode(int) = 0;
};

} // namespace audioapi
