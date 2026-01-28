#pragma once

#include <audioapi/core/utils/AudioDestructor.hpp>

#include <concepts>
#include <memory>
#include <mutex>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include <audioapi/utils/SpscChannel.hpp>

namespace audioapi {

class AudioNode;
class AudioScheduledSourceNode;
class AudioParam;
class AudioBus;

#define AUDIO_GRAPH_MANAGER_SPSC_OPTIONS \
  std::unique_ptr<Event>, channels::spsc::OverflowStrategy::WAIT_ON_FULL, \
      channels::spsc::WaitStrategy::BUSY_LOOP

class AudioGraphManager {
 public:
  enum class ConnectionType { CONNECT, DISCONNECT, DISCONNECT_ALL, ADD };
  typedef ConnectionType EventType;
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

    EventPayload() : nodes{} {}
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

  AudioGraphManager();
  ~AudioGraphManager();

  void preProcessGraph();

  void addPendingNodeConnection(
      const std::shared_ptr<AudioNode> &from,
      const std::shared_ptr<AudioNode> &to,
      ConnectionType type);
  void addPendingParamConnection(
      const std::shared_ptr<AudioNode> &from,
      const std::shared_ptr<AudioParam> &to,
      ConnectionType type);
  void addProcessingNode(const std::shared_ptr<AudioNode> &node);
  void addSourceNode(const std::shared_ptr<AudioScheduledSourceNode> &node);
  void addAudioParam(const std::shared_ptr<AudioParam> &param);

  /// @brief Adds an audio buffer to the manager for destruction.
  /// @note Called directly from the Audio thread (bypasses SPSC).
  void addAudioBusForDestruction(const std::shared_ptr<AudioBus> &bus);

  void cleanup();

 private:
  AudioDestructor<AudioNode> nodeDestructor_;
  AudioDestructor<AudioBus> audioBusDestructor_;

  static constexpr size_t kInitialCapacity = 32;
  static constexpr size_t kChannelCapacity = 1024;

  std::vector<std::shared_ptr<AudioScheduledSourceNode>> sourceNodes_;
  std::vector<std::shared_ptr<AudioNode>> processingNodes_;
  std::vector<std::shared_ptr<AudioParam>> audioParams_;
  std::vector<std::shared_ptr<AudioBus>> audioBuses_;

  channels::spsc::Receiver<AUDIO_GRAPH_MANAGER_SPSC_OPTIONS> receiver_;
  channels::spsc::Sender<AUDIO_GRAPH_MANAGER_SPSC_OPTIONS> sender_;

  void settlePendingConnections();
  void handleConnectEvent(std::unique_ptr<Event> event);
  void handleDisconnectEvent(std::unique_ptr<Event> event);
  void handleDisconnectAllEvent(std::unique_ptr<Event> event);
  void handleAddToDeconstructionEvent(std::unique_ptr<Event> event);

  inline static bool canBeDestructed(const std::shared_ptr<AudioBus> &bus) {
    return bus.use_count() == 1;
  }

  template <typename U>
    requires std::derived_from<U, AudioNode>
  inline static bool canBeDestructed(std::shared_ptr<U> const &node) {
    // If the node is an AudioScheduledSourceNode, we need to check if it is
    // playing
    if constexpr (std::is_base_of_v<AudioScheduledSourceNode, U>) {
      return node.use_count() == 1 && (node->isUnscheduled() || node->isFinished());
    } else if (node->requiresTailProcessing()) {
      // if the node requires tail processing, its own implementation handles disabling it at the right time
      return node.use_count() == 1 && !node->isEnabled();
    }
    return node.use_count() == 1;
  }

  template <typename T, typename D>
    requires std::convertible_to<T *, D *>
  void prepareForDestruction(
      std::vector<std::shared_ptr<T>> &vec,
      AudioDestructor<D> &audioDestructor) {
    if (vec.empty()) {
      return;
    }

    /// An example of input-output
    /// for simplicity we will be considering vector where each value represents
    /// use_count() of an element vec = [1, 2, 1, 3, 1] our end result will be vec
    /// = [2, 3, 1, 1, 1] After this operation all nodes with use_count() == 1
    /// will be at the end and we will try to send them After sending, we will
    /// only keep nodes with use_count() > 1 or which failed vec = [2, 3, failed,
    /// sent, sent] // failed will be always before sents vec = [2, 3, failed] and
    /// we resize
    /// @note if there are no nodes with use_count() == 1 `begin` will be equal to
    /// vec.size()
    /// @note if all nodes have use_count() == 1 `begin` will be 0

    int begin = 0;
    int end = vec.size() - 1; // can be -1 (edge case)

    while (begin <= end) {
      while (begin < end && AudioGraphManager::canBeDestructed(vec[end])) {
        end--;
      }
      if (AudioGraphManager::canBeDestructed(vec[begin])) {
        std::swap(vec[begin], vec[end]);
        end--;
      }
      begin++;
    }

    for (int i = begin; i < vec.size(); i++) {
      if constexpr (std::derived_from<T, AudioNode>) {
        if (vec[i])
          vec[i]->cleanup();
      }

      /// If we fail to add we can't safely remove the node from the vector
      /// so we swap it and advance begin cursor
      /// @note vec[i] does NOT get moved out if it is not successfully added.
      if (!audioDestructor.tryAddForDeconstruction(std::move(vec[i]))) {
        std::swap(vec[i], vec[begin]);
        begin++;
      }
    }

    if (begin < vec.size()) {
      // it does not reallocate if newer size is < current size
      vec.resize(begin);
    }
  }
};

#undef AUDIO_GRAPH_MANAGER_SPSC_OPTIONS

} // namespace audioapi
