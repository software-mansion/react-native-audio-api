#pragma once

#include <audioapi/HostObjects/sources/AudioScheduledSourceNodeHostObject.h>
#include <audioapi/core/sources/AudioFileSourceNode.h>
#include <memory>

namespace audioapi {
using namespace facebook;

struct AudioFileSourceOptions;
class BaseAudioContext;

class AudioFileSourceNodeHostObject : public AudioScheduledSourceNodeHostObject {
 public:
  explicit AudioFileSourceNodeHostObject(
      const std::shared_ptr<BaseAudioContext> &context,
      AudioFileSourceOptions &options);
  ~AudioFileSourceNodeHostObject() override;
  DELETE_COPY_AND_MOVE(AudioFileSourceNodeHostObject);

  JSI_PROPERTY_GETTER_DECL(volume);
  JSI_PROPERTY_GETTER_DECL(loop);
  JSI_PROPERTY_GETTER_DECL(currentTime);
  JSI_PROPERTY_GETTER_DECL(duration);
  JSI_PROPERTY_GETTER_DECL(routedThroughMediaElement);

  JSI_PROPERTY_SETTER_DECL(volume);
  JSI_PROPERTY_SETTER_DECL(loop);
  JSI_PROPERTY_SETTER_DECL(onPositionChanged);

  JSI_HOST_FUNCTION_DECL(pause);
  JSI_HOST_FUNCTION_DECL(seekToStart);
  JSI_HOST_FUNCTION_DECL(seekToTime);

  [[nodiscard]] std::shared_ptr<AudioFileSourceNode> getAudioFileSourceNode() const {
    return std::static_pointer_cast<AudioFileSourceNode>(node_);
  }

 private:
  bool loop_;
  double duration_;
  float volume_;
};

} // namespace audioapi
