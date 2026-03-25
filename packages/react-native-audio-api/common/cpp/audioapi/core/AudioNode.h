#pragma once

#include <audioapi/core/BaseAudioContext.h>
#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
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

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioNodeOptions &options = AudioNodeOptions());
  virtual ~AudioNode();

  size_t getChannelCount() const;
  void connect(const std::shared_ptr<AudioNode> &node);
  void connect(const std::shared_ptr<AudioParam> &param);
  void disconnect();
  void disconnect(const std::shared_ptr<AudioNode> &node);
  void disconnect(const std::shared_ptr<AudioParam> &param);
  virtual std::shared_ptr<DSPAudioBuffer> processAudio(
      const std::shared_ptr<DSPAudioBuffer> &outputBuffer,
      int framesToProcess,
      bool checkIsAlreadyProcessed);

  float getContextSampleRate() const {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->getSampleRate();
    }

    return DEFAULT_SAMPLE_RATE;
  }

  float getNyquistFrequency() const {
    return getContextSampleRate() / 2.0f;
  }

  /// @note JS Thread only
  bool isEnabled() const;
  /// @note JS Thread only
  bool requiresTailProcessing() const;

  template <typename F>
  bool inline scheduleAudioEvent(F &&event) noexcept {
    if (std::shared_ptr<BaseAudioContext> context = context_.lock()) {
      return context->scheduleAudioEvent(std::forward<F>(event));
    }

    return false;
  }

  virtual bool canBeDestructed() const;

 protected:
  friend class AudioGraphManager;
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

  std::unordered_set<AudioNode *> inputNodes_ = {};
  std::unordered_set<std::shared_ptr<AudioNode>> outputNodes_ = {};
  std::unordered_set<std::shared_ptr<AudioParam>> outputParams_ = {};

  int numberOfEnabledInputNodes_ = 0;
  std::atomic<bool> isInitialized_ = false;

  std::size_t lastRenderedFrame_{SIZE_MAX};

  void enable();
  virtual void disable();

 private:
  bool isEnabled_ = true;
  std::vector<std::shared_ptr<DSPAudioBuffer>> inputBuffers_ = {};

  virtual std::shared_ptr<DSPAudioBuffer> processInputs(
      const std::shared_ptr<DSPAudioBuffer> &outputBuffer,
      int framesToProcess,
      bool checkIsAlreadyProcessed);
  virtual std::shared_ptr<DSPAudioBuffer> processNode(
      const std::shared_ptr<DSPAudioBuffer> &,
      int) = 0;

  bool isAlreadyProcessed();
  std::shared_ptr<DSPAudioBuffer> applyChannelCountMode(
      const std::shared_ptr<DSPAudioBuffer> &processingBuffer);
  void mixInputsBuffers(const std::shared_ptr<DSPAudioBuffer> &processingBuffer);

  void connectNode(const std::shared_ptr<AudioNode> &node);
  void disconnectNode(const std::shared_ptr<AudioNode> &node);
  void connectParam(const std::shared_ptr<AudioParam> &param);
  void disconnectParam(const std::shared_ptr<AudioParam> &param);

  void onInputEnabled();
  virtual void onInputDisabled();
  void onInputConnected(AudioNode *node);
  void onInputDisconnected(AudioNode *node);

  void cleanup();
};

} // namespace audioapi
