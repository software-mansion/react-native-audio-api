#pragma once

#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>
#include <audioapi/utils/AudioBuffer.hpp>

#include <memory>

namespace audioapi {
using namespace facebook;

struct AudioBufferSourceOptions;
class BaseAudioContext;
class AudioBufferHostObject;

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

 protected:
  bool loop_;
  bool loopSkip_;
  double loopStart_;
  double loopEnd_;

  void setBuffer(const std::shared_ptr<AudioBuffer> &buffer);
};

} // namespace audioapi
