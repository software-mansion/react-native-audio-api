#pragma once

#include <audioapi/jsi/HostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/ImmutableBufferCache.h>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

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
      immutableCopyCache_ = std::move(other.immutableCopyCache_);
      returnedChannelDataArrays_ = std::move(other.returnedChannelDataArrays_);
    }
    return *this;
  }

  ~AudioBufferHostObject() override = default;

  [[nodiscard]] size_t getSizeInBytes() const {
    // *2 because every time buffer is passed we create a copy of it.
    return audioBuffer_->getSize() * audioBuffer_->getNumberOfChannels() * sizeof(float) * 2;
  }

  /// @brief Returns a defensive copy of `audioBuffer_` suitable for handing to an
  /// `AudioBufferSourceNode`, reusing a cached copy across repeated `.buffer = x`
  /// reassignments of this same JS-visible buffer (e.g. seeking, which recreates the
  /// source node but keeps reusing the already-decoded buffer). Without this, every
  /// reassignment allocated a brand-new full-size copy, which is where
  /// https://github.com/software-mansion/react-native-audio-api/issues/1263 came from.
  /// @note The cache is dropped whenever `audioBuffer_` may have diverged from it:
  /// `copyToChannel` mutates in place, `getChannelData` hands out a live JS-writable
  /// view, and `detachReturnedChannelData` is the last moment such a view could have
  /// been written through.
  [[nodiscard]] std::shared_ptr<AudioBuffer> getOrCreateImmutableCopy() {
    return immutableCopyCache_.getOrCreate(audioBuffer_);
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
    jsi::WeakObject array;
  };

  utils::ImmutableBufferCache immutableCopyCache_;
  /// Float32Array views handed out by `getChannelData` since the last
  /// `detachReturnedChannelData`, kept so they can be neutralised then.
  std::vector<ReturnedChannelDataArray> returnedChannelDataArrays_;
};
} // namespace audioapi
