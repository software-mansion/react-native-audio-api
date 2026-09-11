#pragma once

#include <audioapi/jsi/HostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <jsi/jsi.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

/// @brief JS-facing AudioBuffer. Source nodes play its channel storage directly instead of
/// deep-copying it (see `shareForPlayback`), so the one rule this class enforces is that the
/// JS thread never writes into storage a node may be reading: every write path first gives
/// the touched channel fresh storage (copy-on-write) and leaves the old one to the nodes.
class AudioBufferHostObject : public HostObject {
 public:
  std::shared_ptr<AudioBuffer> audioBuffer_;

  explicit AudioBufferHostObject(const std::shared_ptr<AudioBuffer> &audioBuffer);
  AudioBufferHostObject(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject &operator=(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject(AudioBufferHostObject &&other) noexcept;
  AudioBufferHostObject &operator=(AudioBufferHostObject &&other) noexcept {
    if (this != &other) {
      HostObject::operator=(std::move(other));
      audioBuffer_ = std::move(other.audioBuffer_);
      channelSharedWithNode_ = std::move(other.channelSharedWithNode_);
      contentVersion_ = other.contentVersion_;
      returnedChannelDataArrays_ = std::move(other.returnedChannelDataArrays_);
    }
    return *this;
  }

  ~AudioBufferHostObject() override = default;

  [[nodiscard]] size_t getSizeInBytes() const {
    return audioBuffer_->getSize() * audioBuffer_->getNumberOfChannels() * sizeof(float);
  }

  /// @brief from this call both the js and native side can read the same channel storage.
  /// The JS side is copy-on-write, so it will get a fresh copy if it writes into it while a node is reading it.
  [[nodiscard]] std::shared_ptr<AudioBuffer> shareForPlayback();

  /// @brief Bumped every time a channel's storage is replaced. A source node compares it
  /// with the version it shared at to decide whether "acquire the content" must re-share.
  [[nodiscard]] uint64_t getContentVersion() const {
    return contentVersion_;
  }

  /// @brief Web Audio's "acquire the content" step for the views handed out by
  /// `getChannelData`. Call once playback of this buffer has been scheduled. Every
  /// previously returned Float32Array stops aliasing `audioBuffer_` and, if JS still
  /// holds it, reads as zero-length; the next `getChannelData` call hands out a fresh
  /// view, mirroring what a browser does when it detaches those ArrayBuffers.
  void detachReturnedChannelData(jsi::Runtime &runtime);

  /// @brief Whether any `getChannelData` view is live, i.e. handed out since the last
  /// `detachReturnedChannelData`.
  [[nodiscard]] bool hasReturnedChannelData() const {
    return !returnedChannelDataArrays_.empty();
  }

  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(length);
  JSI_PROPERTY_GETTER_DECL(duration);
  JSI_PROPERTY_GETTER_DECL(numberOfChannels);

  JSI_HOST_FUNCTION_DECL(getChannelData);
  JSI_HOST_FUNCTION_DECL(copyFromChannel);
  JSI_HOST_FUNCTION_DECL(copyToChannel);

 private:
  struct ReturnedChannelDataArray {
    size_t channel;
    /// Weak on purpose: a view JS already dropped must not be kept alive (nor keep its
    /// external-memory-pressure hint alive) until the next start(). The channel is still
    /// recorded so its storage gets swapped, since wrappers over the same ArrayBuffer may
    /// outlive this particular Float32Array object.
    jsi::WeakObject array;
  };

  /// Copy-on-write: call before exposing or mutating a channel from JS. If a node may be
  /// reading that channel's storage, the buffer gets a private copy of it first.
  void makeChannelWritable(size_t channel);

  /// Replaces the channel's storage with a copy and records that nothing shares it yet.
  void replaceChannelStorage(size_t channel);

  /// One flag per channel: true while a node handed out by `shareForPlayback` may still
  /// be reading that channel's current storage.
  std::vector<bool> channelSharedWithNode_;
  uint64_t contentVersion_ = 0;
  /// Float32Array views handed out by `getChannelData` since the last
  /// `detachReturnedChannelData`, kept so they can be neutralised then.
  std::vector<ReturnedChannelDataArray> returnedChannelDataArrays_;
};
} // namespace audioapi
