#pragma once

#include <ReactCommon/CallInvoker.h>
#include <audioapi/jsi/HostObject.h>
#include <audioapi/jsi/JsiPromise.h>
#include <audioapi/utils/AudioRecorderOptions.h>
#include <audioapi/utils/Macros.h>
#include <audioapi/utils/Result.hpp>
#include <audioapi/utils/SerialTaskExecutor.hpp>

#include <memory>
#include <string>

namespace audioapi {
using namespace facebook;

class AudioRecorder;
class IAudioEventHandlerRegistry;
class RecorderOutput;

class AudioRecorderHostObject : public HostObject {
 public:
  AudioRecorderHostObject(
      const std::shared_ptr<IAudioEventHandlerRegistry> &audioEventHandlerRegistry,
      jsi::Runtime *runtime,
      const std::shared_ptr<react::CallInvoker> &callInvoker,
      AudioRecorderOptions options);
  ~AudioRecorderHostObject() override;
  DELETE_COPY_AND_MOVE(AudioRecorderHostObject);

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

  JSI_PROPERTY_GETTER_DECL(inputLatency);

 private:
  /// Swaps @p current for @p replacement on the recorder (either may be null). @p current is left
  /// null when the replacement cannot be added.
  Result<NoneType, std::string> replaceOutput(
      std::shared_ptr<RecorderOutput> &current,
      std::shared_ptr<RecorderOutput> replacement);

  std::shared_ptr<IAudioEventHandlerRegistry> audioEventHandlerRegistry_;
  std::shared_ptr<AudioRecorder> audioRecorder_;
  std::shared_ptr<RecorderOutput> audioReadyOutput_;
  std::shared_ptr<RecorderOutput> adapterOutput_;
  /// Lifecycle calls run FIFO here. Declared after audioRecorder_: the lane destructor drains
  /// queued tasks (including the stop queued by ~AudioRecorderHostObject) before joining.
  std::shared_ptr<SerialTaskExecutor> lifecycleLane_;
  /// Promise vendor over lifecycleLane_.
  std::shared_ptr<PromiseVendor> promiseVendor_;
};

} // namespace audioapi
