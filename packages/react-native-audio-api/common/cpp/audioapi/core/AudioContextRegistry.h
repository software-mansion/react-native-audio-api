#pragma once

#include <audioapi/utils/Macros.h>

#include <memory>
#include <mutex>
#include <vector>

namespace audioapi {

class AudioContext;

/// @brief The process-wide list of live realtime audio contexts.
///
/// The JavaScript host objects own the contexts. Native code that reacts to system events, such
/// as the end of an audio session interruption, still needs to reach every context. This
/// registry keeps non-owning references, so it never extends a context's lifetime.
class AudioContextRegistry {
 public:
  static AudioContextRegistry &global();
  DELETE_COPY_AND_MOVE(AudioContextRegistry);
  ~AudioContextRegistry() = default;

  void add(const std::shared_ptr<AudioContext> &context);

  /// @brief The contexts still alive, in registration order. Holding the returned pointers keeps
  /// them alive, so callers should drop them as soon as they are done.
  [[nodiscard]] std::vector<std::shared_ptr<AudioContext>> liveContexts();

 private:
  AudioContextRegistry() = default;

  std::mutex mutex_;
  std::vector<std::weak_ptr<AudioContext>> contexts_;
};

} // namespace audioapi
