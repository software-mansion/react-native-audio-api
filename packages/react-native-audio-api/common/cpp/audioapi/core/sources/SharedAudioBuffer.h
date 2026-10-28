#pragma once

#include <audioapi/utils/AudioArrayBuffer.hpp>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstddef>
#include <memory>
#include <vector>

namespace audioapi {

/// @brief Samples of an AudioBuffer, shared between the control thread and the source
/// nodes playing them. Nodes read the channel storage directly instead of deep-copying it,
/// so a js trying to write invokes copy-on-write and leaves the old one to the nodes.
/// All members must be called on the JS thread.
class SharedAudioBuffer {
 public:
  explicit SharedAudioBuffer(std::shared_ptr<AudioBuffer> samples);

  [[nodiscard]] std::shared_ptr<const AudioBuffer> getSamples() const {
    return samples_;
  }

  [[nodiscard]] size_t getSizeInBytes() const {
    return samples_->getSize() * samples_->getNumberOfChannels() * sizeof(float);
  }

  /// @brief Storage of @p channel for a JS-writable view (`getChannelData`). Writes through
  /// it reach this buffer until the next `acquireContent` cuts the view off.
  [[nodiscard]] std::shared_ptr<AudioArrayBuffer> exposeChannel(size_t channel);

  /// @brief An out-of-range (or negative) @p startInChannel copies nothing, and a copy that
  /// would run past the end of the channel is truncated rather than failing. @p startInChannel
  /// is the unconverted JS number, so a negative value is rejected instead of wrapping around.
  /// https://webaudio.github.io/web-audio-api/#dom-audiobuffer-copyfromchannel
  void copyFromChannel(
      float *destination,
      size_t destinationLength,
      size_t channel,
      double startInChannel) const;

  /// @brief Same range rules as `copyFromChannel`.
  void
  copyToChannel(const float *source, size_t sourceLength, size_t channel, double startInChannel);

  /// @brief Web Audio's "acquire the content" step, run when a source node schedules
  /// playback of this buffer. Cuts off every view handed out by `exposeChannel`, then
  /// returns a buffer sharing the current storage for the node to play, so JS writes made
  /// before this call are played and later ones are not.
  /// https://webaudio.github.io/web-audio-api/#acquire-the-content
  [[nodiscard]] std::shared_ptr<AudioBuffer> acquireContent();

 private:
  std::shared_ptr<AudioBuffer> samples_;

  /// One flag per channel: true while a buffer returned by `acquireContent` may still be
  /// reading that channel's current storage.
  std::vector<bool> channelSharedWithNode_;
  /// One flag per channel: true once `exposeChannel` handed out that channel's current
  /// storage, cleared when `acquireContent` cuts the view off.
  std::vector<bool> channelExposed_;

  /// Moves @p channel to fresh storage if a node may be reading its current one; afterwards
  /// the channel is safe to write or expose.
  void makeChannelWritable(size_t channel);

  /// Gives the channel fresh storage holding a copy of its samples, shared with nothing.
  void replaceChannelStorage(size_t channel);
};

} // namespace audioapi
