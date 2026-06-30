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
      const std::shared_ptr<utils::graph::Graph> &graph,
      std::unique_ptr<AudioNode> node,
      const BaseAudioBufferSourceOptions &options);
  ~AudioBufferBaseSourceNodeHostObject() override;
  DELETE_COPY_AND_MOVE(AudioBufferBaseSourceNodeHostObject);

  JSI_PROPERTY_GETTER_DECL(detune);
  JSI_PROPERTY_GETTER_DECL(playbackRate);
  JSI_PROPERTY_GETTER_DECL(onPositionChangedInterval);

  JSI_PROPERTY_SETTER_DECL(onPositionChanged);
  JSI_PROPERTY_SETTER_DECL(onPositionChangedInterval);

  JSI_HOST_FUNCTION_DECL(getInputLatency);
  JSI_HOST_FUNCTION_DECL(getOutputLatency);

 protected:
  AudioBufferBaseSourceNode *bufferBaseSourceNode_ = nullptr;

  std::shared_ptr<AudioParamHostObject> detuneParam_;
  std::shared_ptr<AudioParamHostObject> playbackRateParam_;

  int onPositionChangedInterval_;
  double inputLatency_ = 0;
  double outputLatency_ = 0;
  bool pitchCorrection_;

  void initStretch(int channelCount, float sampleRate);
};

} // namespace audioapi
