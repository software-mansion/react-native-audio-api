#pragma once

#include <audioapi/HostObjects/AudioParamHostObject.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {
using namespace facebook;

struct AudioBufferSourceOptions;
class BaseAudioContext;
class SharedAudioBuffer;
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

  /// The buffer assigned last, held here until "acquire the content" hands its samples to
  /// the audio thread. Until then the node has not seen it, so JS can keep writing into it
  /// and every write is honoured. Null when cleared.
  std::shared_ptr<SharedAudioBuffer> assignedBuffer_;
  bool hasBeenStarted_ = false;

  /// The samples the node will read (storage shared with the assigned buffer) plus its
  /// render-quantum scratch buffer.
  struct NodeBuffers {
    std::shared_ptr<AudioBuffer> nodeBuffer;
    std::shared_ptr<DSPAudioBuffer> audioBuffer;
  };

  NodeBuffers prepareNodeBuffers(const std::shared_ptr<AudioBuffer> &acquiredContent);

  /// Records the assignment and publishes the buffer's channel count to the graph. The
  /// samples themselves reach the node only through `acquireBufferContent`, which runs
  /// right away when the node has already started.
  void setBuffer(const std::shared_ptr<SharedAudioBuffer> &buffer);

  /// Web Audio's "acquire the content" step, the single point where the node receives
  /// samples: runs on start(), and on setBuffer() once already started.
  void acquireBufferContent();
};

} // namespace audioapi
