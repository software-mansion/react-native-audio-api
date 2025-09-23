#pragma once

#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/sources/AudioBufferSourceNode.h>
#include <audioapi/HostObjects/sources/AudioBufferBaseSourceNodeHostObject.h>

#include <memory>
#include <vector>

namespace audioapi {
using namespace facebook;

class AudioBufferSourceNodeHostObject
    : public AudioBufferBaseSourceNodeHostObject {
 public:
  explicit AudioBufferSourceNodeHostObject(
      const std::shared_ptr<AudioBufferSourceNode> &node)
      : AudioBufferBaseSourceNodeHostObject(node) {
    addGetters(
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loop),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopSkip),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, buffer),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopStart),
        JSI_EXPORT_PROPERTY_GETTER(AudioBufferSourceNodeHostObject, loopEnd));

    addSetters(
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loop),
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopSkip),
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopStart),
        JSI_EXPORT_PROPERTY_SETTER(AudioBufferSourceNodeHostObject, loopEnd));

    // start method is overridden in this class
    functions_->erase("start");

    addFunctions(
        JSI_EXPORT_FUNCTION(AudioBufferSourceNodeHostObject, start),
        JSI_EXPORT_FUNCTION(AudioBufferSourceNodeHostObject, setBuffer));
    }

  JSI_PROPERTY_GETTER_DECL(loop);
  JSI_PROPERTY_GETTER_DECL(loopSkip);
  JSI_PROPERTY_GETTER_DECL(buffer);
  JSI_PROPERTY_GETTER_DECL(loopStart);
  JSI_PROPERTY_GETTER_DECL(loopEnd);

  JSI_PROPERTY_SETTER_DECL(loop);
  JSI_PROPERTY_SETTER_DECL(loopSkip);
  JSI_PROPERTY_SETTER_DECL(loopStart);
  JSI_PROPERTY_SETTER_DECL(loopEnd);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(setBuffer);
};

} // namespace audioapi
