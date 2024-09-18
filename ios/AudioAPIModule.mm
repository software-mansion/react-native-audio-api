#import "AudioAPIModule.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTUtils.h>
#import <jsi/jsi.h>

#import "../common/cpp/AudioAPI/AudioAPIHostObject.h"
#import "../common/cpp/AudioAPI/AudioAPIWrapper.h"

@implementation AudioAPIModule

RCT_EXPORT_MODULE(AudioAPIModule)

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install)
{
  NSLog(@"Installing JSI bindings for react-native-audio-api...");
  RCTBridge *bridge = [RCTBridge currentBridge];
  RCTCxxBridge *cxxBridge = (RCTCxxBridge *)bridge;
  if (cxxBridge == nil) {
    return @false;
  }

  using namespace facebook;

  auto jsiRuntime = (jsi::Runtime *)cxxBridge.runtime;
  if (jsiRuntime == nil) {
    return @false;
  }
  auto &runtime = *jsiRuntime;

  auto wrapper = std::make_shared<audioapi::AudioAPIWrapper>();
  auto hostObject = std::make_shared<audioapi::AudioAPIHostObject>(wrapper);
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);
  runtime.global().setProperty(runtime, "__AudioAPI", std::move(object));

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
  return @true;
}

@end
