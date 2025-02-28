#import "AudioAPIModule.h"

#import <React/RCTBridge+Private.h>

#ifdef RCT_NEW_ARCH_ENABLED
#import <React/RCTCallInvoker.h>
#import <ReactCommon/RCTTurboModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#include <audioapi/AudioAPIModuleInstaller.h>

using namespace audioapi;
using namespace facebook::react;

@interface RCTBridge (JSIRuntime)
- (void *)runtime;
@end

#if defined(RCT_NEW_ARCH_ENABLED) && REACT_NATIVE_MINOR_VERSION >= 75
// nothing
#else // defined(RCT_NEW_ARCH_ENABLED) && REACT_NATIVE_MINOR_VERSION >= 75
@interface RCTBridge (RCTTurboModule)
- (std::shared_ptr<facebook::react::CallInvoker>)jsCallInvoker;
- (void)_tryAndHandleError:(dispatch_block_t)block;
@end
#endif // RCT_NEW_ARCH_ENABLED

@implementation AudioAPIModule

#if defined(RCT_NEW_ARCH_ENABLED)
@synthesize callInvoker = _callInvoker;
#endif // defined(RCT_NEW_ARCH_ENABLED)

RCT_EXPORT_MODULE(AudioAPIModule);

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install)
{
  auto *cxxBridge = reinterpret_cast<RCTCxxBridge *>(self.bridge);
  auto jsiRuntime = reinterpret_cast<facebook::jsi::Runtime *>(cxxBridge.runtime);

#if defined(RCT_NEW_ARCH_ENABLED)
  auto jsCallInvoker = _callInvoker.callInvoker;
#else // defined(RCT_NEW_ARCH_ENABLED)
  auto jsCallInvoker = self.bridge.jsCallInvoker;
#endif // defined(RCT_NEW_ARCH_ENABLED)

  assert(jsiRuntime != nullptr);

  audioapi::AudioAPIModuleInstaller::injectJSIBindings(jsiRuntime, jsCallInvoker);

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
  return @true;
}

#ifdef RCT_NEW_ARCH_ENABLED
- (std::shared_ptr<facebook::react::TurboModule>)getTurboModule:
    (const facebook::react::ObjCTurboModule::InitParams &)params
{
  return std::make_shared<facebook::react::NativeAudioAPIModuleSpecJSI>(params);
}
#endif // RCT_NEW_ARCH_ENABLED

@end
