#import "AudioContextModule.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTUtils.h>
#import <jsi/jsi.h>

#import "../cpp/AudioContextHostObject.h"

@implementation AudioContextModule

RCT_EXPORT_MODULE(AudioContext)

RCT_EXPORT_BLOCKING_SYNCHRONOUS_METHOD(install)
{
    NSLog(@"Installing JSI bindings for react-native-audio-context...");
    RCTBridge* bridge = [RCTBridge currentBridge];
    RCTCxxBridge* cxxBridge = (RCTCxxBridge*)bridge;
    if (cxxBridge == nil) {
        return @false;
    }

    using namespace facebook;

    auto jsiRuntime = (jsi::Runtime*) cxxBridge.runtime;
    if (jsiRuntime == nil) {
        return @false;
    }
    auto& runtime = *jsiRuntime;

    auto hostObject = std::make_shared<audiocontext::AudioContextHostObject>();
    auto object = jsi::Object::createFromHostObject(runtime, hostObject);
    runtime.global().setProperty(runtime, "__AudioContextProxy", std::move(object));

    NSLog(@"Successfully installed JSI bindings for react-native-audio-context!");
    return @true;
}

@end
