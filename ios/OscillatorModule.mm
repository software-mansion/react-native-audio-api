#import "OscillatorModule.h"

#import <React/RCTBridge+Private.h>
#import <React/RCTUtils.h>
#import <jsi/jsi.h>

#import "../cpp/OscillatorHostObject.h"

@implementation OscillatorModule

RCT_EXPORT_MODULE(Oscillator)

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

    auto hostFunctionCreateOscillator = [](jsi::Runtime&
        runtime, const jsi::Value& thisVal, const jsi::Value* args, size_t count) -> jsi::Value {
        const float frequency = static_cast<float>(args[0].asNumber());

        auto cppOscillatorHostObjectPtr = std::make_shared<audiocontext::OscillatorHostObject>(frequency);

        const auto &jsiOscillatorHostObject = jsi::Object::createFromHostObject(runtime, cppOscillatorHostObjectPtr);

        return jsi::Value(runtime, jsiOscillatorHostObject);
    };

    auto jsiCreateOscillator = jsi::Function::createFromHostFunction(runtime, jsi::PropNameID::forUtf8(runtime, "__OscillatorProxy"), 0, hostFunctionCreateOscillator);

    runtime.global().setProperty(runtime, jsi::String::createFromUtf8(runtime, "__OscillatorProxy"), jsiCreateOscillator);

    NSLog(@"Successfully installed JSI bindings for react-native-audio-context!");
    return @true;
}

@end
