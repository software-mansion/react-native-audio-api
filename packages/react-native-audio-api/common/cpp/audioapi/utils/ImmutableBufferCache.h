#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi::utils {

/// @brief Caches a defensive copy of an `AudioBuffer` so repeated requests for
/// "an immutable copy suitable for handing to a source node" reuse the same
/// copy instead of allocating a fresh one every time. Used by
/// `AudioBufferHostObject` to avoid deep-copying the entire buffer on every
/// `.buffer = x` reassignment of the same underlying buffer. That reassignment
/// is what seeking requires, since a live `AudioBufferSourceNode` can't be
/// repositioned. See https://github.com/software-mansion/react-native-audio-api/issues/1263.
/// @note `invalidate()` must be called whenever the source buffer's data could
/// have diverged from the cached copy. This class has no way to observe that on
/// its own.
class ImmutableBufferCache {
 public:
  /// @brief Returns a defensive copy of `source`, reusing the last copy
  /// produced as long as `invalidate()` has not been called since.
  [[nodiscard]] std::shared_ptr<AudioBuffer> getOrCreate(
      const std::shared_ptr<AudioBuffer> &source) {
    if (cached_ == nullptr) {
      cached_ = std::make_shared<AudioBuffer>(*source);
    }
    return cached_;
  }

  /// @brief Call when the source buffer's data may no longer match the cached
  /// copy: it was mutated in place (`copyToChannel`), a live JS-writable view
  /// into it was handed out (`getChannelData`), or such a view has just been
  /// cut off after possibly being written through. The next `getOrCreate`
  /// produces a fresh copy and caching resumes from there.
  void invalidate() {
    cached_ = nullptr;
  }

 private:
  std::shared_ptr<AudioBuffer> cached_;
};

} // namespace audioapi::utils
