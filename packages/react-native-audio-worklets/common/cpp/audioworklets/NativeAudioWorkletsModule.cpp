#include <audioworklets/AudioWorkletsInstaller.h>
#include <audioworklets/NativeAudioWorkletsModule.h>

#include <memory>
#include <utility>

namespace facebook::react {

NativeAudioWorkletsModule::NativeAudioWorkletsModule(std::shared_ptr<CallInvoker> jsInvoker)
    : NativeAudioWorkletsModuleCxxSpec(std::move(jsInvoker)) {}

bool NativeAudioWorkletsModule::install(jsi::Runtime &runtime) {
  audioworklets::AudioWorkletsInstaller::inject(runtime);
  return true;
}

} // namespace facebook::react
