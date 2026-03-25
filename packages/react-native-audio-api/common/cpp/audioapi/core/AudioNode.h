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
#include <unordered_set>
#include <utility>
#include <vector>

namespace audioapi {

class AudioParam;

class AudioNode : public utils::graph::GraphObject, public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioNodeOptions &options = AudioNodeOptions());
  virtual ~AudioNode();

  size_t getChannelCount() const;

  template <std::ranges::input_range R>
    requires std::same_as<std::ranges::range_reference_t<R>, const GraphObject &>
  void process(R &&inputs, int numFrames) {
    audioBuffer_->zero();

    for (const auto &input : inputs) {
      if (const AudioNode *audioNode = input.asAudioNode()) {
        audioBuffer_->sum(*audioNode->audioBuffer_, channelInterpretation_);
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

  std::shared_ptr<DSPAudioBuffer> getAudioBuffer() const {
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
  friend class AudioDestinationNode;
  friend class ConvolverNode;
  friend class DelayNodeHostObject;

  std::weak_ptr<BaseAudioContext> context_;
  std::shared_ptr<DSPAudioBuffer> audioBuffer_;

  const int numberOfInputs_ = 1;
  const int numberOfOutputs_ = 1;
  size_t channelCount_ = 2;
  const ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  const ChannelInterpretation channelInterpretation_ = ChannelInterpretation::SPEAKERS;
  const bool requiresTailProcessing_;

  std::atomic<bool> isInitialized_ = false;

  std::size_t lastRenderedFrame_{SIZE_MAX};

  virtual void disable() {
    cleanup();
  };

  virtual void processNode(int) = 0;

  void cleanup();
};

} // namespace audioapi
