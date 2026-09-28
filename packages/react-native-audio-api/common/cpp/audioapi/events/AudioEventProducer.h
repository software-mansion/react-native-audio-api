#pragma once

#include <audioapi/libs/concurrentqueue/concurrentqueue.h>
#include <audioapi/utils/Macros.h>

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
  /// @param queue The registry's dispatch queue; the token binds to it for its whole life.
  template <typename TQueue>
  explicit AudioEventProducer(TQueue &queue) : token_(queue) {}

  ~AudioEventProducer() = default;

  DELETE_COPY_AND_MOVE(AudioEventProducer);

  [[nodiscard]] moodycamel::ProducerToken &token() noexcept {
    return token_;
  }

 private:
  moodycamel::ProducerToken token_;
};

} // namespace audioapi
