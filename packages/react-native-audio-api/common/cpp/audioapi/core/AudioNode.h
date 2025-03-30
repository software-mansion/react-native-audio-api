#pragma once

#include <audioapi/core/types/ChannelCountMode.h>
#include <audioapi/core/types/ChannelInterpretation.h>
#include <audioapi/core/Constants.h>

#include <memory>
#include <string>
#include <cstddef>
#include <vector>
#include <cassert>

namespace audioapi {

class AudioBus;
class BaseAudioContext;

class AudioNode : public std::enable_shared_from_this<AudioNode> {
 public:
  explicit AudioNode(BaseAudioContext *context);
  virtual ~AudioNode();

  int getNumberOfInputs() const;
  int getNumberOfOutputs() const;
  int getChannelCount() const;
  std::string getChannelCountMode() const;
  std::string getChannelInterpretation() const;
  void connect(const std::shared_ptr<AudioNode> &node);
  void disconnect();
  void disconnect(const std::shared_ptr<AudioNode> &node);

  bool isEnabled() const;
  void enable();
  void disable();

 protected:
  friend class AudioNodeManager;
  friend class AudioDestinationNode;

  BaseAudioContext *context_;
  std::shared_ptr<AudioBus> audioBus_;

  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  int channelCount_ = 2;
  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ =
          ChannelInterpretation::SPEAKERS;

  std::vector<AudioNode *> inputNodes_ = {};
  std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};

  int numberOfEnabledInputNodes_ = 0;
  bool isInitialized_ = false;
  bool isEnabled_ = true;

  std::size_t lastRenderedFrame_{SIZE_MAX};

 private:
  friend class StretcherNode;

  std::vector<std::shared_ptr<AudioBus>> inputBuses_ = {};

  static std::string toString(ChannelCountMode mode);
  static std::string toString(ChannelInterpretation interpretation);

  virtual std::shared_ptr<AudioBus> processAudio(const std::shared_ptr<AudioBus> &outputBus, int framesToProcess, bool checkIsAlreadyProcessed);
  virtual void processNode(const std::shared_ptr<AudioBus>&, int) = 0;

  bool isAlreadyProcessed();
  std::shared_ptr<AudioBus> processInputs(const std::shared_ptr<AudioBus> &outputBus, int framesToProcess, bool checkIsAlreadyProcessed);
  std::shared_ptr<AudioBus> applyChannelCountMode(std::shared_ptr<AudioBus> &processingBus);
  void mixInputsBuses(const std::shared_ptr<AudioBus> &processingBus);

  void connectNode(const std::shared_ptr<AudioNode> node);
  void disconnectNode(const std::shared_ptr<AudioNode> node, bool notifyNode);

  void onInputEnabled();
  void onInputDisabled();
  void onInputConnected(AudioNode *node);
  void onInputDisconnected(AudioNode *node);

  void cleanup(bool shouldThrow);
};

} // namespace audioapi
