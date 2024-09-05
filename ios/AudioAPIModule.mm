#import "AudioAPIModule.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTUtils.h>
#import <jsi/jsi.h>

#import "../common/cpp/AudioContext/AudioContextHostObject.h"
#import "../common/cpp/AudioContext/AudioContextWrapper.h"

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

  auto wrapper = std::make_shared<audioapi::AudioContextWrapper>();
  auto hostObject = std::make_shared<audioapi::AudioContextHostObject>(wrapper);
  auto object = jsi::Object::createFromHostObject(runtime, hostObject);
  runtime.global().setProperty(runtime, "__AudioContext", std::move(object));

  NSLog(@"Successfully installed JSI bindings for react-native-audio-api!");
  return @true;
}

@end
