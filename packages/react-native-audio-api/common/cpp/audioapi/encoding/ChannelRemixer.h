#pragma once

#include <audioapi/core/utils/Constants.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <array>
#include <cstddef>
#include <memory>

namespace audioapi {

/// Brings encoder input to the file's channel count with the Web Audio mixing rules
class ChannelRemixer {
 public:
  using ChannelPointers = std::array<const float *, MAX_CHANNEL_COUNT>;

  /// A remixer that passes every channel through, for an encoder that has no input yet.
  ChannelRemixer() = default;

  /// Allocates the staging buffers when the counts differ; @p maxFrames bounds a single remix().
  ChannelRemixer(int inputChannelCount, int outputChannelCount, size_t maxFrames, float sampleRate);

  /// Returns outputChannelCount() channel pointers holding @p frames frames. Without a remix they
  /// are @p channels themselves; otherwise they point into this remixer's buffer and stay valid
  /// until the next call. @p frames must not exceed the constructor's maxFrames.
  [[nodiscard]] ChannelPointers remix(const float *const *channels, size_t frames);

  [[nodiscard]] bool isRemixing() const {
    return remixedPlanar_ != nullptr;
  }

  [[nodiscard]] int outputChannelCount() const {
    return outputChannelCount_;
  }

 private:
  int inputChannelCount_ = 0;
  int outputChannelCount_ = 0;
  std::unique_ptr<AudioBuffer> inputPlanar_;
  std::unique_ptr<AudioBuffer> remixedPlanar_;
};

} // namespace audioapi
