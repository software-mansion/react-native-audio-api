#pragma once

#include <audioapi/jsi/JsiHostObject.h>

#include <memory>
#include <utility>
#include <vector>
#include <cstdio>

namespace audioapi {
using namespace facebook;

class AudioRecorder;
class AudioEventHandlerRegistry;

class AudioRecorderHostObject : public JsiHostObject {
 public:
  explicit AudioRecorderHostObject(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      int bufferLength);

  JSI_PROPERTY_SETTER_DECL(onAudioReady);

  JSI_HOST_FUNCTION_DECL(connect);
  JSI_HOST_FUNCTION_DECL(disconnect);
  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(stop);

 private:
  std::shared_ptr<AudioRecorder> audioRecorder_;
};
} // namespace audioapi
