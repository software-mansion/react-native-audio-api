#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <memory>

namespace audioapi {

/// Brings encoder input to the file's channel count with the Web Audio mixing rules
class ChannelRemixer {
 public:
  /// A remixer that passes every channel through, for an encoder that has no input yet.
  ChannelRemixer() = default;

  /// Allocates the staging buffer when the counts differ; @p maxFrames bounds a single remix().
  ChannelRemixer(int inputChannelCount, int outputChannelCount, size_t maxFrames, float sampleRate);

  /// Returns the first @p frames frames of @p input in the output channel count: @p input itself
  /// when nothing has to be remixed, otherwise this remixer's buffer, which stays valid until the
  /// next call. @p frames must not exceed the constructor's maxFrames.
  [[nodiscard]] const AudioBuffer &remix(const AudioBuffer &input, size_t frames);

  [[nodiscard]] bool isRemixing() const {
    return remixed_ != nullptr;
  }

  [[nodiscard]] int outputChannelCount() const {
    return outputChannelCount_;
  }

 private:
  int outputChannelCount_ = 0;
  std::unique_ptr<AudioBuffer> remixed_;
};

} // namespace audioapi
