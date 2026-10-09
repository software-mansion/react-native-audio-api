#pragma once

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

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
  JSI_PROPERTY_SETTER_DECL(onloopended);

  JSI_HOST_FUNCTION_DECL(start) override;
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

  /// The JS-facing buffer assigned last, held here until "acquire the content" hands its
  /// samples to the audio thread. Until then the node has not seen it, so JS can keep
  /// writing into it and every write is honoured. Null when cleared.
  std::shared_ptr<AudioBufferHostObject> assignedBufferHostObject_;
  bool hasBeenStarted_ = false;

  /// The samples the node will read (storage shared with the JS-facing buffer) plus its
  /// render-quantum scratch buffer.
  struct NodeBuffers {
    std::shared_ptr<AudioBuffer> nodeBuffer;
    std::shared_ptr<DSPAudioBuffer> audioBuffer;
  };

  NodeBuffers prepareNodeBuffers(const std::shared_ptr<AudioBufferHostObject> &bufferHostObject);

  /// Records the assignment and publishes the buffer's channel count to the graph. The
  /// samples themselves reach the node only through `acquireBufferContent`, which runs
  /// right away when the node has already started.
  void setBuffer(const std::shared_ptr<AudioBufferHostObject> &bufferHostObject);

  /// Web Audio's "acquire the content" step, the single point where the node receives
  /// samples: runs on start(), and on setBuffer() once already started. Cuts off every
  /// live getChannelData() view, then hands the node the buffer's content as it is now,
  /// so JS writes made before this moment are played and later ones are not.
  /// https://webaudio.github.io/web-audio-api/#acquire-the-content
  void acquireBufferContent();
};

} // namespace audioapi
