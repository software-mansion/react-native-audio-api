#import "AudioAPIModule.h"

#import <React/RCTBridge+Private.h>
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
    return @false;
  }

  using namespace facebook;

  auto jsiRuntime = (jsi::Runtime *)cxxBridge.runtime;
  if (jsiRuntime == nil) {
    return @false;
  }
  auto &runtime = *jsiRuntime;

  auto wrapper = std::make_shared<audioapi::AudioAPIInstallerWrapper>();
  auto hostObject = std::make_shared<audioapi::AudioAPIInstallerHostObject>(wrapper);
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);
  runtime.global().setProperty(runtime, "__AudioAPIInstaller", std::move(object));

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
  return @true;
}

@end
