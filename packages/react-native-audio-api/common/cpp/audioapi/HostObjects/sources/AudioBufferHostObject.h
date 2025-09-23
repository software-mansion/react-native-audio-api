#pragma once

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/jsi/AudioArrayBuffer.h>
#include <audioapi/jsi/JsiHostObject.h>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferHostObject : public JsiHostObject {
 public:
  std::shared_ptr<AudioBuffer> audioBuffer_;

  explicit AudioBufferHostObject(
      const std::shared_ptr<AudioBuffer> &audioBuffer)
      : audioBuffer_(audioBuffer) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, sampleRate),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, length),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, duration),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferHostObject, numberOfChannels));

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioBufferHostObject, getChannelData),
        JSI_EXPORT_FUNCTION(AudioBufferHostObject, copyFromChannel),
        JSI_EXPORT_FUNCTION(AudioBufferHostObject, copyToChannel));
  }
  AudioBufferHostObject(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject &operator=(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject(AudioBufferHostObject &&other) noexcept
      : JsiHostObject(std::move(other)),
        audioBuffer_(std::move(other.audioBuffer_)) {}
  AudioBufferHostObject &operator=(AudioBufferHostObject &&other) noexcept {
    if (this != &other) {
      JsiHostObject::operator=(std::move(other));
      audioBuffer_ = std::move(other.audioBuffer_);
    }
    return *this;
  }

  [[nodiscard]] inline size_t getSizeInBytes() const {
    return audioBuffer_->getLength() * audioBuffer_->getNumberOfChannels() *
        sizeof(float);
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
