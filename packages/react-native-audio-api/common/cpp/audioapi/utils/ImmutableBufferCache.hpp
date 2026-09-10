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
/// @note The two invalidation methods must be called whenever the source
/// buffer's data could have been mutated from JS. This class has no way to
/// observe that on its own.
class ImmutableBufferCache {
 public:
  /// @brief Returns a defensive copy of `source`, reusing the last copy
  /// produced as long as neither `invalidate()` nor `markLiveViewEscaped()`
  /// has been called since.
  [[nodiscard]] std::shared_ptr<AudioBuffer> getOrCreate(
      const std::shared_ptr<AudioBuffer> &source) {
    if (liveViewEscaped_) {
      // A live, JS-writable view into source's data has escaped. A write
      // through it could happen at any later time, so we can no longer prove
      // a cached copy won't go stale. Fall back to copying every time.
      return std::make_shared<AudioBuffer>(*source);
    }

    if (cached_ == nullptr) {
      cached_ = std::make_shared<AudioBuffer>(*source);
    }
    return cached_;
  }

  /// @brief Call when the source buffer's data has just been mutated in place
  /// (e.g. `copyToChannel`). The current cached copy is now stale.
  void invalidate() {
    cached_ = nullptr;
  }

  /// @brief Call when a live, JS-writable view into the source buffer's data
  /// has been handed out (e.g. `getChannelData`). This permanently stops
  /// caching, since the view can be written through at any later time, not
  /// just at the moment it was retrieved.
  void markLiveViewEscaped() {
    liveViewEscaped_ = true;
    cached_ = nullptr;
  }

 private:
  std::shared_ptr<AudioBuffer> cached_;
  bool liveViewEscaped_ = false;
};

} // namespace audioapi::utils
