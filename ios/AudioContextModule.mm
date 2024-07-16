#import "AudioContextModule.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTUtils.h>
#import <jsi/jsi.h>

#import "../cpp/OscillatorHostObject.h"

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

    runtime.global().setProperty(
        runtime,
        jsi::String::createFromUtf8(runtime, "createOscillator"),
        jsi::Function::createFromHostFunction(
            runtime,
            jsi::PropNameID::forUtf8(runtime, "createOscillator"),
            0,
            [](jsi::Runtime& runtime, const jsi::Value& thisVal, const jsi::Value* args, size_t count) -> jsi::Value {
                const float frequency = static_cast<float>(args[0].asNumber());

                auto cppOscillatorHostObjectPtr = std::make_shared<audiocontext::OscillatorHostObject>(frequency);

                const auto& jsiOscillatorHostObject = jsi::Object::createFromHostObject(runtime, cppOscillatorHostObjectPtr);

                return jsi::Value(runtime, jsiOscillatorHostObject);
            }
        )
    );

    NSLog(@"Successfully installed JSI bindings for react-native-audio-context!");
    return @true;
}

@end
