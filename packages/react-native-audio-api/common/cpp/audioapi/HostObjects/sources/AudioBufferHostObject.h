#pragma once

#include <audioapi/core/sources/SharedAudioBuffer.h>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <jsi/jsi.h>
#include <cstddef>
#include <memory>

namespace audioapi {
using namespace facebook;

class AudioBufferHostObject : public HostObject {
 public:
  std::shared_ptr<SharedAudioBuffer> sharedBuffer_;

  explicit AudioBufferHostObject(const std::shared_ptr<AudioBuffer> &audioBuffer);
  AudioBufferHostObject(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject &operator=(const AudioBufferHostObject &) = delete;
  AudioBufferHostObject(AudioBufferHostObject &&other) noexcept = default;
  AudioBufferHostObject &operator=(AudioBufferHostObject &&other) noexcept = default;

  ~AudioBufferHostObject() override = default;

  [[nodiscard]] size_t getSizeInBytes() const {
    return sharedBuffer_->getSizeInBytes();
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
