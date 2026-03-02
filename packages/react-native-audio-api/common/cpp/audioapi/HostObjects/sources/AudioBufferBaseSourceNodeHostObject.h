#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioBufferBaseSourceNode;
struct BaseAudioBufferSourceOptions;
class AudioParamHostObject;

class AudioBufferBaseSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioBufferBaseSourceNodeHostObject(
      const std::shared_ptr<AudioBufferBaseSourceNode> &node,
      const BaseAudioBufferSourceOptions &options);

  ~AudioBufferBaseSourceNodeHostObject() override;

  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(playbackRate);
  JSI_PROPERTY_GETTER_DECL(onPositionChangedInterval);

  JSI_PROPERTY_SETTER_DECL(onPositionChanged);
  JSI_PROPERTY_SETTER_DECL(onPositionChangedInterval);

  JSI_HOST_FUNCTION_DECL(getInputLatency);
  JSI_HOST_FUNCTION_DECL(getOutputLatency);

 protected:
  std::shared_ptr<AudioParamHostObject> detuneParam_;
  std::shared_ptr<AudioParamHostObject> playbackRateParam_;

  int onPositionChangedInterval_;
  uint64_t onPositionChangedCallbackId_ = 0;

  double inputLatency_ = 0;
  double outputLatency_ = 0;
  bool pitchCorrection_;

  void setOnPositionChangedCallbackId(uint64_t callbackId);
  void initStretch(int channelCount, float sampleRate);
};

} // namespace audioapi
