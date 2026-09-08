#pragma once

#include <audioapi/utils/AudioBuffer.hpp>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>

#if RN_AUDIO_API_TEST

namespace audioapi {

/// @brief Test double for the C++ test build, which compiles no HostObjects: under
/// RN_AUDIO_API_TEST an audio buffer does not inherit jsi::MutableBuffer (see
/// AudioArrayBuffer.hpp), so the real class's JSI accessors would not compile.
class AudioBufferHostObject : public facebook::jsi::HostObject {
 public:
  std::shared_ptr<AudioBuffer> audioBuffer_;

  explicit AudioBufferHostObject(const std::shared_ptr<AudioBuffer> &audioBuffer)
      : audioBuffer_(audioBuffer) {}

  [[nodiscard]] size_t getSizeInBytes() const {
    return audioBuffer_->getSize() * audioBuffer_->getNumberOfChannels() * sizeof(float) * 2;
  }
};

} // namespace audioapi

#else

#include <audioapi/jsi/HostObject.h>

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

  JSI_PROPERTY_GETTER_DECL(sampleRate);
  JSI_PROPERTY_GETTER_DECL(length);
  JSI_PROPERTY_GETTER_DECL(duration);
  JSI_PROPERTY_GETTER_DECL(numberOfChannels);

  JSI_HOST_FUNCTION_DECL(getChannelData);
  JSI_HOST_FUNCTION_DECL(copyFromChannel);
  JSI_HOST_FUNCTION_DECL(copyToChannel);
};
} // namespace audioapi

#endif // RN_AUDIO_API_TEST
