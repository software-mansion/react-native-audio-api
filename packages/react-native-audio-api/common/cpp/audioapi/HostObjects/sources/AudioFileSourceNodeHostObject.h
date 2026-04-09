#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <memory>

namespace audioapi {
using namespace facebook;

struct AudioFileSourceOptions;
class BaseAudioContext;

class AudioFileSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioFileSourceNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      const AudioFileSourceOptions &options);

  ~AudioFileSourceNodeHostObject() override;

  JSI_PROPERTY_GETTER_DECL(volume);
  JSI_PROPERTY_GETTER_DECL(loop);
  JSI_PROPERTY_GETTER_DECL(currentTime);
  JSI_PROPERTY_GETTER_DECL(duration);

  JSI_PROPERTY_SETTER_DECL(volume);
  JSI_PROPERTY_SETTER_DECL(loop);
  JSI_PROPERTY_SETTER_DECL(onPositionChanged);

  JSI_HOST_FUNCTION_DECL(pause);
  JSI_HOST_FUNCTION_DECL(seekToStart);
  JSI_HOST_FUNCTION_DECL(seekToTime);

 private:
  uint64_t onPositionChangedCallbackId_ = 0;
  uint64_t onEndedCallbackId_ = 0;

  void setOnPositionChangedCallbackId(uint64_t callbackId);
  void setOnEndedCallbackId(uint64_t callbackId);

  bool loop_;
  double duration_;
  float volume_;
};

} // namespace audioapi
