#pragma once

#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferSourceNode;

class AudioBufferSourceNodeHostObject : public AudioBufferBaseSourceNodeHostObject {
 public:
  explicit AudioBufferSourceNodeHostObject(const std::shared_ptr<AudioBufferSourceNode> &node);

  ~AudioBufferSourceNodeHostObject() override;

  JSI_PROPERTY_GETTER_DECL(loop);
  JSI_PROPERTY_GETTER_DECL(loopSkip);
  JSI_PROPERTY_GETTER_DECL(buffer);
  JSI_PROPERTY_GETTER_DECL(loopStart);
  JSI_PROPERTY_GETTER_DECL(loopEnd);

  JSI_PROPERTY_SETTER_DECL(loop);
  JSI_PROPERTY_SETTER_DECL(loopSkip);
  JSI_PROPERTY_SETTER_DECL(loopStart);
  JSI_PROPERTY_SETTER_DECL(loopEnd);
  JSI_PROPERTY_SETTER_DECL(onLoopEnded);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(setBuffer);
};

} // namespace audioapi
