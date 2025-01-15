#import "AudioAPIModule.h"

#ifdef RCT_NEW_ARCH_ENABLED
#import <React/RCTBridge+Private.h>
#import <React/RCTCallInvoker.h>
#import <ReactCommon/RCTTurboModule.h>
#endif // RCT_NEW_ARCH_ENABLED

#import "AudioAPIInstallerHostObject.h"

@implementation AudioAPIModule

#if defined(RCT_NEW_ARCH_ENABLED)
// nothing
#else // defined(RCT_NEW_ARCH_ENABLED)
@interface RCTBridge (RCTTurboModule)
- (std::shared_ptr<facebook::react::CallInvoker>)jsCallInvoker;
- (void)_tryAndHandleError:(dispatch_block_t)block;
@end
#endif // RCT_NEW_ARCH_ENABLED

#if defined(RCT_NEW_ARCH_ENABLED)
@synthesize callInvoker = _callInvoker;
#endif // defined(RCT_NEW_ARCH_ENABLED)

RCT_EXPORT_MODULE(AudioAPIModule)

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

  auto hostObject = std::make_shared<audioapi::AudioAPIInstallerHostObject>(jsiRuntime, jsCallInvoker);
  hostObject->install();

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
  return @true;
}

@end
