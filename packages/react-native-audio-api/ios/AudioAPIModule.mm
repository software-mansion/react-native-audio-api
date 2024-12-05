#import "AudioAPIModule.h"

#import <ReactCommon/RCTTurboModule.h>
#import <React/RCTBridge+Private.h>
#import <React/RCTBridge.h>
#import <React/RCTUtils.h>
#import <jsi/jsi.h>

#import "AudioAPIInstallerHostObject.h"
#import "AudioAPIInstallerWrapper.h"

@implementation AudioAPIModule

RCT_EXPORT_MODULE(AudioAPIModule)

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install)
{
  NSLog(@"Installing JSI bindings for react-native-audio-api...");
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)self.bridge;

  if (cxxBridge == nil) {
    NSLog(@"Error during getting bridge!");
    return @false;
  }

  using namespace facebook;

  auto jsRuntime = (jsi::Runtime *)cxxBridge.runtime;

  if (jsRuntime == nil) {
    NSLog(@"Error during getting jsRuntime!");
    return @false;
  }

  auto &runtime = *jsRuntime;

  auto wrapper = std::make_shared<audioapi::AudioAPIInstallerWrapper>();
  auto hostObject = std::make_shared<audioapi::AudioAPIInstallerHostObject>(wrapper, jsRuntime, cxxBridge.jsCallInvoker);
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);
  runtime.global().setProperty(runtime, "__AudioAPIInstaller", std::move(object));

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
  return @true;
}

@end
