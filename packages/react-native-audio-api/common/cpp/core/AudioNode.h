#pragma once

#include <memory>
#include <string>
#include <unordered_set>
#include <cstddef>

#include "ChannelCountMode.h"
#include "ChannelInterpretation.h"
#include "Constants.h"

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

  std::unordered_set<AudioNode *> inputNodes_ = {};
  std::unordered_set<std::shared_ptr<AudioNode>> outputNodes_ = {};

  int numberOfEnabledInputNodes_ = 0;
  bool isInitialized_ = false;
  bool isEnabled_ = true;

  std::size_t lastRenderedFrame_{SIZE_MAX};

 private:
  std::vector<AudioBus*> inputBuses_;

  static std::string toString(ChannelCountMode mode);
  static std::string toString(ChannelInterpretation interpretation);

  AudioBus *processAudio(AudioBus *outputBus, int framesToProcess);
  virtual void processNode(AudioBus *processingBus, int framesToProcess) = 0;

  bool isAlreadyProcessed();
  AudioBus *processInputs(AudioBus *outputBus, int framesToProcess);
  AudioBus *applyChannelCountMode(AudioBus *processingBus);
  void mixInputsBuses(AudioBus *processingBus);

  void connectNode(const std::shared_ptr<AudioNode> &node);
  void disconnectNode(const std::shared_ptr<AudioNode> &node);

  void onInputEnabled();
  void onInputDisabled();
  void onInputConnected(AudioNode *node);
  void onInputDisconnected(AudioNode *node);

  void cleanup();
};

} // namespace audioapi
