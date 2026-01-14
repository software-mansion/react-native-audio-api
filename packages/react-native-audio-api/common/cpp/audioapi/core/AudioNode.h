#pragma once

#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/utils/Constants.h>

#include <cassert>
#include <cstddef>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

namespace audioapi {

class AudioBus;
class BaseAudioContext;
class AudioParam;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(std::shared_ptr<BaseAudioContext> context);
  virtual ~AudioNode();

  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNode> &node);
  void connect(const std::shared_ptr<AudioParam> &param);
  void disconnect();
  void disconnect(const std::shared_ptr<AudioNode> &node);
  void disconnect(const std::shared_ptr<AudioParam> &param);
  virtual std::shared_ptr<AudioBus> processAudio(
      const std::shared_ptr<AudioBus> &outputBus,
      int framesToProcess,
      bool checkIsAlreadyProcessed);

  bool isEnabled() const;
  bool requiresTailProcessing() const;
  void enable();
  virtual void disable();

 protected:
  friend class AudioNodeManager;
  friend class AudioDestinationNode;
  friend class ConvolverNode;
  friend class DelayNodeHostObject;
  int channelCount_ = 2;

  std::weak_ptr<BaseAudioContext> context_;
  std::shared_ptr<AudioBus> audioBus_;

  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ =

      ChannelInterpretation::SPEAKERS;

  std::unordered_set<AudioNode *> inputNodes_ = {};
  std::unordered_set<std::shared_ptr<AudioNode>> outputNodes_ = {};
  std::unordered_set<std::shared_ptr<AudioParam>> outputParams_ = {};

  int numberOfEnabledInputNodes_ = 0;
  bool isInitialized_ = false;
  bool isEnabled_ = true;
  bool requiresTailProcessing_ = false;

  std::size_t lastRenderedFrame_{SIZE_MAX};

 private:
  std::vector<std::shared_ptr<AudioBus>> inputBuses_ = {};

  static std::string toString(ChannelCountMode mode);
  static std::string toString(ChannelInterpretation interpretation);

  virtual std::shared_ptr<AudioBus> processInputs(
      const std::shared_ptr<AudioBus> &outputBus,
      int framesToProcess,
      bool checkIsAlreadyProcessed);
  virtual std::shared_ptr<AudioBus> processNode(const std::shared_ptr<AudioBus> &, int) = 0;

  bool isAlreadyProcessed();
  std::shared_ptr<AudioBus> applyChannelCountMode(const std::shared_ptr<AudioBus> &processingBus);
  void mixInputsBuses(const std::shared_ptr<AudioBus> &processingBus);

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
