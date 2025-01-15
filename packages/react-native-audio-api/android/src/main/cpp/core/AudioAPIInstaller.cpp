#include "AudioAPIInstaller.h"

namespace audioapi {

using namespace facebook::jni;

AudioAPIInstaller::AudioAPIInstaller(
    jni::alias_ref<AudioAPIInstaller::jhybridobject> &jThis,
    jsi::Runtime *jsiRuntime,
    const std::shared_ptr<facebook::react::CallInvoker> &jsCallInvoker)
    : javaPart_(make_global(jThis)),
      jsiRuntime_(jsiRuntime),
      jsCallInvoker_(jsCallInvoker) {}

jni::local_ref<AudioAPIInstaller::jhybriddata> AudioAPIInstaller::initHybrid(
    jni::alias_ref<jhybridobject> jThis,
    jlong jsContext,
    jni::alias_ref<facebook::react::CallInvokerHolder::javaobject>
        jsCallInvokerHolder) {
  auto jsCallInvoker = jsCallInvokerHolder->cthis()->getCallInvoker();
  auto rnRuntime = reinterpret_cast<jsi::Runtime *>(jsContext);
  return makeCxxInstance(jThis, rnRuntime, jsCallInvoker);
}

void AudioAPIInstaller::registerNatives() {
  registerHybrid({
      makeNativeMethod("initHybrid", AudioAPIInstaller::initHybrid),
      makeNativeMethod("install", AudioAPIInstaller::install),
  });
}

void AudioAPIInstaller::install() {
  auto hostObject = std::make_shared<AudioAPIInstallerHostObject>(
      jsiRuntime_, jsCallInvoker_);
  hostObject->install();
}
} // namespace audioapi
