#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/core/utils/graph/GraphObject.hpp>
#include <audioapi/types/NodeOptions.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cassert>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {

class AudioParam;

class AudioNode : public utils::graph::GraphObject, public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioNodeOptions &options = AudioNodeOptions());

  size_t getChannelCount() const;

  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_reference_t<R>, const GraphObject &>
  void process(R &&inputs, int numFrames) {
    getInputBuffer()->zero();

    for (const auto &input : inputs) {
      if (const AudioNode *audioNode = input.asAudioNode()) {
        getInputBuffer()->sum(*audioNode->getOutputBuffer(), channelInterpretation_);
      }
    }

    processNode(numFrames);
  }

  float getContextSampleRate() const {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->getSampleRate();
    }

    return DEFAULT_SAMPLE_RATE;
  }

  float getNyquistFrequency() const {
    return getContextSampleRate() / 2.0f;
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
  bool requiresTailProcessing() const;

  template <typename F>
  bool inline scheduleAudioEvent(F &&event) noexcept {
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
  //  friend class ConvolverNode;
  friend class DelayNodeHostObject;

  std::weak_ptr<BaseAudioContext> context_;
  std::shared_ptr<DSPAudioBuffer> audioBuffer_;

  const int numberOfInputs_ = 1;
  const int numberOfOutputs_ = 1;
  size_t channelCount_ = 2;
  const ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  const ChannelInterpretation channelInterpretation_ = ChannelInterpretation::SPEAKERS;
  const bool requiresTailProcessing_;

  virtual void processNode(int) = 0;
};

} // namespace audioapi
