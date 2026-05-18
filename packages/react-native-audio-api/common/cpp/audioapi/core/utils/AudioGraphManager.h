#pragma once

#include <audioapi/core/utils/AudioDestructor.hpp>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/SpscChannel.hpp>
#include <concepts>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {

class AudioNode;
class AudioScheduledSourceNode;
class AudioParam;

#define AUDIO_GRAPH_MANAGER_SPSC_OPTIONS \
  std::unique_ptr<Event>, channels::spsc::OverflowStrategy::WAIT_ON_FULL, \
      channels::spsc::WaitStrategy::BUSY_LOOP

template <typename T>
concept HasCleanupMethod = requires(T t) {
  { t.cleanup() };
};

class AudioGraphManager {
 public:
  enum class ConnectionType : uint8_t { CONNECT, DISCONNECT, DISCONNECT_ALL, ADD };
  using EventType = ConnectionType; // for backwards compatibility
  enum class EventPayloadType : uint8_t { NODES, PARAMS, SOURCE_NODE, AUDIO_PARAM, NODE };
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
    EventType type = ConnectionType::CONNECT;
    EventPayloadType payloadType = EventPayloadType::NODES;
    EventPayload payload;

    Event(Event &&other) noexcept;
    Event &operator=(Event &&other) noexcept;
    Event() = default;
    ~Event();
  };

  AudioGraphManager();
  DELETE_COPY_AND_MOVE(AudioGraphManager);
  ~AudioGraphManager();

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

  /// @brief Adds an audio buffer to the manager for destruction.
  /// @note Called directly from the Audio thread (bypasses SPSC).
  void addAudioBufferForDestruction(std::shared_ptr<AudioBuffer> buffer);

  void cleanup();

 private:
  AudioDestructor<AudioNode> nodeDestructor_;
  AudioDestructor<AudioBuffer> bufferDestructor_;

  /// @brief Initial capacity for various node types for deletion
  /// @note Higher capacity decreases number of reallocations at runtime (can be easily adjusted to 128 if needed)
  static constexpr size_t kInitialCapacity = 32;

  /// @brief Initial capacity for event passing channel
  /// @note High value reduces wait time for sender (JavaScript/HostObjects thread here)
  static constexpr size_t kChannelCapacity = 1024;

  std::vector<std::shared_ptr<AudioScheduledSourceNode>> sourceNodes_;
  std::vector<std::shared_ptr<AudioNode>> processingNodes_;
  std::vector<std::shared_ptr<AudioParam>> audioParams_;
  std::vector<std::shared_ptr<AudioBuffer>> audioBuffers_;

  channels::spsc::Receiver<AUDIO_GRAPH_MANAGER_SPSC_OPTIONS> receiver_;

  channels::spsc::Sender<AUDIO_GRAPH_MANAGER_SPSC_OPTIONS> sender_;

  void settlePendingConnections();
  static void handleConnectEvent(std::unique_ptr<Event> event);
  static void handleDisconnectEvent(std::unique_ptr<Event> event);
  static void handleDisconnectAllEvent(std::unique_ptr<Event> event);
  void handleAddToDeconstructionEvent(std::unique_ptr<Event> event);

  template <typename U>
  static bool canBeDestructed(const std::shared_ptr<U> &object) {
    return object.use_count() == 1;
  }

  template <typename U>
    requires std::derived_from<U, AudioNode>
  static bool canBeDestructed(std::shared_ptr<U> const &node) {
    // If the node is an AudioScheduledSourceNode, we need to check if it is
    // playing
    if constexpr (std::is_base_of_v<AudioScheduledSourceNode, U>) {
      return node.use_count() == 1 && (node->isUnscheduled() || node->isFinished());
    }

    if (node->requiresTailProcessing()) {
      // if the node requires tail processing, its own implementation handles disabling it at the right time
      return node.use_count() == 1 && !node->isEnabled();
    }

    return node.use_count() == 1;
  }

  template <typename T, typename D>
    requires std::convertible_to<T *, D *>
  static void prepareForDestruction(
      std::vector<std::shared_ptr<T>> &vec,
      AudioDestructor<D> &audioDestructor) {
    if (vec.empty()) {
      return;
    }
    // Compact in place: try to queue each destructible element; keep the rest.
    size_t keepIndex = 0;
    for (size_t i = 0; i < vec.size(); ++i) {
      if (AudioGraphManager::canBeDestructed(vec[i])) {
        if constexpr (HasCleanupMethod<T>) {
          if (vec[i]) {
            vec[i]->cleanup();
          }
        }

        // vec[i] does NOT get moved out if it is not successfully added.
        if (audioDestructor.tryAddForDeconstruction(std::move(vec[i]))) {
          continue;
        }
      }

      if (keepIndex != i) {
        vec[keepIndex] = std::move(vec[i]);
      }
      ++keepIndex;
    }
    vec.resize(keepIndex);
  }
};

#undef AUDIO_GRAPH_MANAGER_SPSC_OPTIONS

} // namespace audioapi
