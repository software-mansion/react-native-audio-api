#pragma once

#include <audioapi/libs/concurrentqueue/concurrentqueue.h>
#include <audioapi/utils/Macros.h>
#include <optional>

namespace audioapi {

/// @brief One audio thread's private lane into the event registry's dispatch queue.
///
/// A moodycamel ProducerToken is single-producer by construction: do not share tokens between threads.
///
/// every context owns a producer and hands it to
/// IAudioEventHandlerRegistry::dispatchEventFromAudioThread.
///
/// @note A producer may move between threads over time — an OfflineAudioContext spawns a
/// fresh render thread on every resume — but never concurrently
class AudioEventProducer {
 public:
  /// @param queue The registry's dispatch queue; the token binds to it until release().
  template <typename TQueue>
  explicit AudioEventProducer(TQueue &queue) : token_(queue) {}

  ~AudioEventProducer() = default;

  DELETE_COPY_AND_MOVE(AudioEventProducer);

  /// @return The token to enqueue through; empty once released.
  [[nodiscard]] std::optional<moodycamel::ProducerToken> &token() noexcept {
    return token_;
  }

  /// @brief Hands the token, and the queue blocks it holds, back to the queue for the next
  /// producer to reuse. Nodes keep this object alive long after their context is done, so
  /// waiting for the destructor would strand the blocks; dispatching afterwards fails.
  /// @note Only once no thread can dispatch through this producer any more.
  void release() noexcept {
    token_.reset();
  }

 private:
  std::optional<moodycamel::ProducerToken> token_;
};

} // namespace audioapi
