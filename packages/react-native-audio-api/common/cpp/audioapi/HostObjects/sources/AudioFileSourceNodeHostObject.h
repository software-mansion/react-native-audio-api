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

  ~AudioFileSourceNodeHostObject() override = default;

  JSI_PROPERTY_GETTER_DECL(volume);
  JSI_PROPERTY_SETTER_DECL(volume);
  JSI_PROPERTY_GETTER_DECL(loop);
  JSI_PROPERTY_SETTER_DECL(loop);
  JSI_PROPERTY_GETTER_DECL(currentTime);
  JSI_PROPERTY_GETTER_DECL(duration);

  JSI_HOST_FUNCTION_DECL(pause);
};

} // namespace audioapi
