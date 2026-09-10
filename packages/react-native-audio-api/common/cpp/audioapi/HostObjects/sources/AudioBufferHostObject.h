#pragma once

#include <audioapi/jsi/HostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>
#include <audioapi/utils/ImmutableBufferCache.hpp>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <utility>

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
  /// @note The cache is invalidated whenever the buffer's data could have been mutated
  /// from JS (`copyToChannel`, or ever having handed out a live `getChannelData` view),
  /// since a cached copy must never be shared while its source can still be written to.
  [[nodiscard]] std::shared_ptr<AudioBuffer> getOrCreateImmutableCopy() {
    return immutableCopyCache_.getOrCreate(audioBuffer_);
  }

  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(length);
  JSI_PROPERTY_GETTER_DECL(duration);
  JSI_PROPERTY_GETTER_DECL(numberOfChannels);

  JSI_HOST_FUNCTION_DECL(getChannelData);
  JSI_HOST_FUNCTION_DECL(copyFromChannel);
  JSI_HOST_FUNCTION_DECL(copyToChannel);

 private:
  utils::ImmutableBufferCache immutableCopyCache_;
};
} // namespace audioapi
