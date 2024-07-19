#ifdef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext {

    jsi::Value AudioContextWrapper::createOscillator(jsi::Runtime& runtime, const jsi::PropNameID& propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) -> jsi::Value {
            auto oscillator = audiocontext_->createOscillator();
            return jsi::Object::createFromHostObject(runtime, oscillator);
        });
    }

    jsi::Value AudioContextWrapper::getDestination(jsi::Runtime& runtime, const jsi::PropNameID& propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) -> jsi::Value {
            auto destination = audiocontext_->getDestination();
            return jsi::Object::createFromHostObject(runtime, destination);
        });
    }
} // namespace audiocontext
#endif