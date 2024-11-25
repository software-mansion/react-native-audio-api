#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Constants.h"
#include "ChannelCountMode.h"
#include "ChannelInterpretation.h"

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
  void disconnect(const std::shared_ptr<AudioNode> &node);

  bool isEnabled() const;
  void enable();
  void disable();

 protected:
  friend class AudioNodeManager;
  friend class AudioDestinationNode;

  BaseAudioContext *context_;
  std::shared_ptr<AudioBus> audioBus_;

  int channelCount_ = CHANNEL_COUNT;

  int numberOfInputs_ = 1;
  int numberOfOutputs_ = 1;
  int numberOfEnabledInputNodes_ = 0;

  bool isEnabled_ = true;

  std::size_t lastRenderedFrame_ { SIZE_MAX };

  ChannelCountMode channelCountMode_ = ChannelCountMode::MAX;
  ChannelInterpretation channelInterpretation_ =
      ChannelInterpretation::SPEAKERS;

  std::vector<AudioNode*> inputNodes_ = {};
  std::vector<std::shared_ptr<AudioNode>> outputNodes_ = {};

 private:
  static std::string toString(ChannelCountMode mode);
  static std::string toString(ChannelInterpretation interpretation);

  void cleanup();
  AudioBus* processAudio(AudioBus* outputBus, int framesToProcess);
  virtual void processNode(AudioBus* processingBus, int framesToProcess) = 0;

  void connectNode(std::shared_ptr<AudioNode> &node);
  void disconnectNode(std::shared_ptr<AudioNode> &node);

  void onInputEnabled();
  void onInputDisabled();
  void onInputConnected(AudioNode *node);
  void onInputDisconnected(AudioNode *node);
};

} // namespace audioapi

/*
Audio node management and deletion

1. AudioSourceNode finishes playing, then:
  - it disables itself (so it won't by processed anymore)
  - if there is no reference from JS side, we can mark self for deletion
  - node that is marked for deletion should be disconnected from the graph first
  - it should "notify" connected nodes that it has been disabled (and potentially prepares for deletion)

2. Node gets "notified" that one of its sources is disabled:
 - it lowers the count of enabled sources
 - if the count of enabled sources is 0, disable itself
 - if there is no reference from JS side, we can mark self for deletion
 - node that is marked for deletion should be disconnected from the graph first
 - it should "notify" connected nodes that it has been disabled (and potentially prepares for deletion)

Translating into more technical terms:
We use shared pointers for keeping output nodes
We use shared pointers in audio node manager to keep track of all source nodes
when audio source node finished playing it:
  - disables itself and tells all output nodes that it has been disabled
  - each node up to destination, checks their input nodes and if was its only active input node, it disables itself.
  - source node tells audio node manager to dereference it (only if it is the last reference to the source node).
  - audio manager in pre-process or post-process will remove the reference.
  - audio manager in pre-process or post-process will also have to check for source nodes with only one reference and delete them if already stopped.
  - deletion of the node will dereference all connected nodes, resulting in destroy'ing them if they are not referenced from JS side.

*/
