#pragma once

#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>
#include <audioapi/types/NodeOptions.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace audioapi {

class AudioBuffer;
class BaseAudioContext;
class AudioParam;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioNodeOptions &options = AudioNodeOptions());
  virtual ~AudioNode();

  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  size_t getChannelCount() const;
  ChannelCountMode getChannelCountMode() const;
  ChannelInterpretation getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNode> &node);
  void connect(const std::shared_ptr<AudioParam> &param);
  void disconnect();
  void disconnect(const std::shared_ptr<AudioNode> &node);
  void disconnect(const std::shared_ptr<AudioParam> &param);
  virtual std::shared_ptr<AudioBuffer> processAudio(
      const std::shared_ptr<AudioBuffer> &outputBuffer,
      int framesToProcess,
      bool checkIsAlreadyProcessed);

  bool isEnabled() const;
  bool requiresTailProcessing() const;
  void enable();
  virtual void disable();

 protected:
  friend class AudioGraphManager;
  friend class AudioDestinationNode;
  friend class ConvolverNode;
  friend class DelayNodeHostObject;

  std::weak_ptr<BaseAudioContext> context_;
  std::shared_ptr<AudioBuffer> audioBuffer_;

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
  bool isInitialized_ = false;
  bool isEnabled_ = true;

  std::size_t lastRenderedFrame_{SIZE_MAX};

 private:
  std::vector<std::shared_ptr<AudioBuffer>> inputBuffers_ = {};

  virtual std::shared_ptr<AudioBuffer> processInputs(
      const std::shared_ptr<AudioBuffer> &outputBuffer,
      int framesToProcess,
      bool checkIsAlreadyProcessed);
  virtual std::shared_ptr<AudioBuffer> processNode(const std::shared_ptr<AudioBuffer> &, int) = 0;

  bool isAlreadyProcessed();
  std::shared_ptr<AudioBuffer> applyChannelCountMode(
      const std::shared_ptr<AudioBuffer> &processingBuffer);
  void mixInputsBuffers(const std::shared_ptr<AudioBuffer> &processingBuffer);

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
