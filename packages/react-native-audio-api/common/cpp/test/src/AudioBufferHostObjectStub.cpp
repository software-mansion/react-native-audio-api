#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>

#include <memory>
#include <utility>

/// The test build compiles no HostObject sources, yet core code it does compile
/// (AudioRecorderCallback, ContextPromiseResolver) constructs an AudioBufferHostObject
/// when a recording or decode finishes. These definitions satisfy the linker in place of
/// AudioBufferHostObject.cpp, which cannot be compiled here: under RN_AUDIO_API_TEST an
/// AudioArrayBuffer is not a jsi::MutableBuffer (see AudioArrayBuffer.hpp), so its JSI
/// accessor bodies do not build. The stub registers no accessors, and no test ever
/// reaches the object through a JS runtime.

namespace audioapi {

AudioBufferHostObject::AudioBufferHostObject(const std::shared_ptr<AudioBuffer> &audioBuffer)
    : audioBuffer_(audioBuffer) {}

AudioBufferHostObject::AudioBufferHostObject(AudioBufferHostObject &&other) noexcept
    : JsiHostObject(std::move(other)), audioBuffer_(std::move(other.audioBuffer_)) {}

} // namespace audioapi
