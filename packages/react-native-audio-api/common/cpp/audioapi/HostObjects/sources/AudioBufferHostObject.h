#pragma once

#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <utility>

namespace audioapi {
using namespace facebook;

class AudioBufferHostObject : public JsiHostObject {
 public:
  std::shared_ptr<AudioBuffer> audioBuffer_;

  explicit AudioBufferHostObject(const std::shared_ptr<AudioBuffer> &audioBuffer);
  AudioBufferHostObject(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject &operator=(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject(AudioBufferHostObject &&other) noexcept;
  AudioBufferHostObject &operator=(AudioBufferHostObject &&other) noexcept {
    if (this != &other) {
      JsiHostObject::operator=(std::move(other));
      audioBuffer_ = std::move(other.audioBuffer_);
    }
    return *this;
  }

  [[nodiscard]] inline size_t getSizeInBytes() const {
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
