#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/jsi/JsiHostObject.h>
#include <audioapi/jsi/JsiPromise.h>

#include <memory>

namespace audioapi {
using namespace facebook;

class AudioRecorder;
class AudioEventHandlerRegistry;

class AudioRecorderHostObject : public JsiHostObject {
 public:
  AudioRecorderHostObject(
      const std::shared_ptr<AudioEventHandlerRegistry> &audioEventHandlerRegistry,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker);

  JSI_HOST_FUNCTION_DECL(start);
  JSI_HOST_FUNCTION_DECL(stop);
  JSI_HOST_FUNCTION_DECL(isRecording);
  JSI_HOST_FUNCTION_DECL(isPaused);

  JSI_HOST_FUNCTION_DECL(enableFileOutput);
  JSI_HOST_FUNCTION_DECL(disableFileOutput);

  JSI_HOST_FUNCTION_DECL(pause);
  JSI_HOST_FUNCTION_DECL(resume);

  JSI_HOST_FUNCTION_DECL(connect);
  JSI_HOST_FUNCTION_DECL(disconnect);

  JSI_HOST_FUNCTION_DECL(setOnAudioReady);
  JSI_HOST_FUNCTION_DECL(clearOnAudioReady);

  JSI_HOST_FUNCTION_DECL(setOnError);
  JSI_HOST_FUNCTION_DECL(clearOnError);

  JSI_HOST_FUNCTION_DECL(getCurrentDuration);

 private:
  std::shared_ptr<AudioRecorder> audioRecorder_;
  std::shared_ptr<PromiseVendor> promiseVendor_;
};

} // namespace audioapi
