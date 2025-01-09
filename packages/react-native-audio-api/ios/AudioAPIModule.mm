#import "AudioAPIModule.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTCallInvoker.h>
#import <ReactCommon/RCTTurboModule.h>

#import "AudioAPIInstallerHostObject.h"

@implementation AudioAPIModule

@synthesize callInvoker = _callInvoker;

RCT_EXPORT_MODULE(AudioAPIModule)

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install)
{
  auto *cxxBridge = reinterpret_cast<RCTCxxBridge *>(self.bridge);
  auto jsiRuntime = reinterpret_cast<facebook::jsi::Runtime *>(cxxBridge.runtime);
  auto jsCallInvoker = _callInvoker.callInvoker;

  assert(jsiRuntime != nullptr);

  auto hostObject = std::make_shared<audioapi::AudioAPIInstallerHostObject>(jsiRuntime, jsCallInvoker);
  hostObject->install();

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
  return @true;
}

@end
