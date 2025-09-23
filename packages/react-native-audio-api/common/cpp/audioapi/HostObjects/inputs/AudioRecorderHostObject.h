#pragma once

#include <jsi/jsi.h>

#include <audioapi/core/sources/AudioBuffer.h>
#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/core/inputs/AudioRecorder.h>
#include <audioapi/HostObjects/sources/RecorderAdapterNodeHostObject.h>

#ifdef ANDROID
#include <audioapi/android/core/AndroidAudioRecorder.h>
#else
#include <audioapi/ios/core/IOSAudioRecorder.h>
#endif

#include <memory>
#include <utility>
#include <vector>
#include <cstdio>

namespace audioapi {
using namespace facebook;

class AudioRecorderHostObject : public JsiHostObject {
 public:
  explicit AudioRecorderHostObject(
      jsi::Runtime *runtime,
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      float sampleRate,
      int bufferLength) {
#ifdef ANDROID
    audioRecorder_ = std::make_shared<AndroidAudioRecorder>(
      sampleRate,
      bufferLength,
      audioEventHandlerRegistry
    );
#else
  audioRecorder_ = std::make_shared<IOSAudioRecorder>(
      sampleRate,
      bufferLength,
      audioEventHandlerRegistry
    );
#endif

    addSetters(JSI_EXPORT_PROPERTY_SETTER(AudioRecorderHostObject, onAudioReady));

    addFunctions(
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, start),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, stop),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, connect),
      JSI_EXPORT_FUNCTION(AudioRecorderHostObject, disconnect)
    );
  }

  JSI_PROPERTY_SETTER_DECL(onAudioReady);

  JSI_HOST_FUNCTION_DECL(connect);
  JSI_HOST_FUNCTION_DECL(disconnect);
  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(stop);

 private:
  std::shared_ptr<AudioRecorder> audioRecorder_;
};
} // namespace audioapi
