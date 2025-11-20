#pragma once

#include <audioapi/core/utils/AudioNodeDestructor.h>

#include <memory>
#include <mutex>
#include <tuple>
#include <vector>

#include <audioapi/utils/SpscChannel.hpp>

namespace audioapi {

class AudioNode;
class AudioScheduledSourceNode;
class AudioParam;

#define AUDIO_NODE_MANAGER_SPSC_OPTIONS \
  std::unique_ptr<Event>, channels::spsc::OverflowStrategy::WAIT_ON_FULL, \
      channels::spsc::WaitStrategy::BUSY_LOOP

class AudioNodeManager {
 public:
  enum class ConnectionType { CONNECT, DISCONNECT, DISCONNECT_ALL, ADD };
  typedef ConnectionType EventType; // for backwards compatibility
  enum class EventPayloadType { NODES, PARAMS, SOURCE_NODE, AUDIO_PARAM, NODE };
  union EventPayload {
    struct {
      std::shared_ptr<AudioNode> from;
      std::shared_ptr<AudioNode> to;
    } nodes;
    struct {
      std::shared_ptr<AudioNode> from;
      std::shared_ptr<AudioParam> to;
    } params;
    std::shared_ptr<AudioScheduledSourceNode> sourceNode;
    std::shared_ptr<AudioParam> audioParam;
    std::shared_ptr<AudioNode> node;

    // Default constructor that initializes the first member
    EventPayload() : nodes{} {}

    // Destructor - we'll handle cleanup explicitly in Event destructor
    ~EventPayload() {}
  };
  struct Event {
    EventType type;
    EventPayloadType payloadType;
    EventPayload payload;

    Event(Event &&other);
    Event &operator=(Event &&other);
    Event() : type(ConnectionType::CONNECT), payloadType(EventPayloadType::NODES), payload() {}
    ~Event();
  };

  AudioNodeManager();
  ~AudioNodeManager();

  void preProcessGraph();

  /// @brief Adds a pending connection between two audio nodes.
  /// @param from The source audio node.
  /// @param to The destination audio node.
  /// @param type The type of connection (connect/disconnect).
  /// @note Should be only used from JavaScript/HostObjects thread
  void addPendingNodeConnection(
      const std::shared_ptr<AudioNode> &from,
      const std::shared_ptr<AudioNode> &to,
      ConnectionType type);

  /// @brief Adds a pending connection between an audio node and an audio parameter.
  /// @param from The source audio node.
  /// @param to The destination audio parameter.
  /// @param type The type of connection (connect/disconnect).
  /// @note Should be only used from JavaScript/HostObjects thread
  void addPendingParamConnection(
      const std::shared_ptr<AudioNode> &from,
      const std::shared_ptr<AudioParam> &to,
      ConnectionType type);

  /// @brief Adds a processing node to the manager.
  /// @param node The processing node to add.
  /// @note Should be only used from JavaScript/HostObjects thread
  void addProcessingNode(const std::shared_ptr<AudioNode> &node);

  /// @brief Adds a source node to the manager.
  /// @param node The source node to add.
  /// @note Should be only used from JavaScript/HostObjects thread
  void addSourceNode(const std::shared_ptr<AudioScheduledSourceNode> &node);

  /// @brief Adds an audio parameter to the manager.
  /// @param param The audio parameter to add.
  /// @note Should be only used from JavaScript/HostObjects thread
  void addAudioParam(const std::shared_ptr<AudioParam> &param);

  void cleanup();

 private:
  AudioNodeDestructor nodeDeconstructor_;

  /// @brief Initial capacity for various node types for deletion
  /// @note Higher capacity decreases number of reallocations at runtime (can be easily adjusted to 128 if needed)
  static constexpr size_t kInitialCapacity = 32;

  /// @brief Initial capacity for event passing channel
  /// @note High value reduces wait time for sender (JavaScript/HostObjects thread here)
  static constexpr size_t kChannelCapacity = 1024;

  std::vector<std::shared_ptr<AudioScheduledSourceNode>> sourceNodes_;
  std::vector<std::shared_ptr<AudioNode>> processingNodes_;
  std::vector<std::shared_ptr<AudioParam>> audioParams_;

  channels::spsc::Receiver<AUDIO_NODE_MANAGER_SPSC_OPTIONS> receiver_;

  channels::spsc::Sender<AUDIO_NODE_MANAGER_SPSC_OPTIONS> sender_;

  void settlePendingConnections();
  void handleConnectEvent(std::unique_ptr<Event> event);
  void handleDisconnectEvent(std::unique_ptr<Event> event);
  void handleDisconnectAllEvent(std::unique_ptr<Event> event);
  void handleAddToDeconstructionEvent(std::unique_ptr<Event> event);

  template <typename U>
  void prepareNodesForDestruction(std::vector<std::shared_ptr<U>> &vec);

  template <typename U>
  inline static bool nodeCanBeDestructed(std::shared_ptr<U> const &node);
};

#undef AUDIO_NODE_MANAGER_SPSC_OPTIONS

} // namespace audioapi
