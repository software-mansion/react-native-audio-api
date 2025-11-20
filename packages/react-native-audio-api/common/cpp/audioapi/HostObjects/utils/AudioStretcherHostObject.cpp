#include <audioapi/HostObjects/sources/AudioBufferHostObject.h>
#include <audioapi/HostObjects/utils/AudioStretcherHostObject.h>
#include <audioapi/core/utils/AudioStretcher.h>
#include <audioapi/jsi/JsiPromise.h>

#include <jsi/jsi.h>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace audioapi {

AudioStretcherHostObject::AudioStretcherHostObject(
    jsi::Runtime *runtime,
    const std::shared_ptr<react::CallInvoker> &callInvoker) {
  promiseVendor_ = std::make_shared<PromiseVendor>(runtime, callInvoker);
  addFunctions(JSI_EXPORT_FUNCTION(AudioStretcherHostObject, changePlaybackSpeed));
}

JSI_HOST_FUNCTION_IMPL(AudioStretcherHostObject, changePlaybackSpeed) {
  auto audioBuffer = args[0].getObject(runtime).asHostObject<AudioBufferHostObject>(runtime);
  auto playbackSpeed = static_cast<float>(args[1].asNumber());

  auto promise = promiseVendor_->createAsyncPromise([=]() -> PromiseResolver {
    auto result = AudioStretcher::changePlaybackSpeed(*audioBuffer->audioBuffer_, playbackSpeed);

    if (result == nullptr) {
      return [](jsi::Runtime &runtime) {
        return std::string("Failed to change audio playback speed.");
      };
    }
    return [result](jsi::Runtime &runtime) {
      auto audioBufferHostObject = std::make_shared<AudioBufferHostObject>(result);
      return jsi::Object::createFromHostObject(runtime, audioBufferHostObject);
    };
  });
  return promise;
}

} // namespace audioapi
