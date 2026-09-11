#pragma once

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <cstdint>
#include <memory>

namespace audioapi {
using namespace facebook;

struct AudioBufferSourceOptions;
class BaseAudioContext;
class AudioBufferHostObject;
class AudioBufferSourceNode;

class AudioBufferSourceNodeHostObject : public AudioBufferBaseSourceNodeHostObject {
 public:
  explicit AudioBufferSourceNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioBufferSourceOptions &options);

  ~AudioBufferSourceNodeHostObject() override;

  JSI_PROPERTY_GETTER_DECL(loop);
  JSI_PROPERTY_GETTER_DECL(loopSkip);
  JSI_PROPERTY_GETTER_DECL(loopStart);
  JSI_PROPERTY_GETTER_DECL(loopEnd);

  JSI_PROPERTY_SETTER_DECL(loop);
  JSI_PROPERTY_SETTER_DECL(loopSkip);
  JSI_PROPERTY_SETTER_DECL(loopStart);
  JSI_PROPERTY_SETTER_DECL(loopEnd);
  JSI_PROPERTY_SETTER_DECL(onLoopEnded);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(setBuffer);

  [[nodiscard]] size_t getMemoryPressure() const override {
    // playbackRate + detune params (owned by the base source). The AudioBuffer
    // itself is tracked separately as an AudioBufferHostObject and re-hinted
    // via setExternalMemoryPressure in `setBuffer`.
    return AudioNodeHostObject::getMemoryPressure() + 2 * kAudioParamBytes;
  }

 protected:
  AudioBufferSourceNode *const audioBufferSourceNode_;

  bool loop_;
  bool loopSkip_;
  double loopStart_;
  double loopEnd_;

  /// The JS-visible buffer behind the last `setBuffer`, kept so the "acquire the
  /// content" step can run on it. Null when the buffer came from options or was cleared.
  std::shared_ptr<AudioBufferHostObject> bufferHostObject_;
  /// `bufferHostObject_->getContentVersion()` at the moment the node last received its
  /// samples. A newer version means JS replaced some channel storage since, so the node
  /// is reading stale content and must be re-handed the buffer when it acquires it.
  uint64_t sharedContentVersion_ = 0;
  bool hasBeenStarted_ = false;

  /// The samples the node will read (shared with the JS-facing buffer when possible, a
  /// padded private copy for pitch correction) plus its render-quantum scratch buffer.
  struct NodeBuffers {
    std::shared_ptr<AudioBuffer> nodeBuffer;
    std::shared_ptr<DSPAudioBuffer> audioBuffer;
  };

  NodeBuffers prepareNodeBuffers(
      const std::shared_ptr<AudioBuffer> &buffer,
      const std::shared_ptr<AudioBufferHostObject> &bufferHostObject);

  void setBuffer(
      const std::shared_ptr<AudioBuffer> &buffer,
      const std::shared_ptr<AudioBufferHostObject> &bufferHostObject = nullptr);

  /// Web Audio's "acquire the content" step: runs on start() when a buffer is set, and on
  /// setBuffer() once already started. Cuts off every live getChannelData() view and
  /// if the JS-facing buffer's storage moved on since the node last received it,
  /// re-hands the node the current content.
  /// https://webaudio.github.io/web-audio-api/#acquire-the-content
  void acquireBufferContent(jsi::Runtime &runtime);
};

} // namespace audioapi
