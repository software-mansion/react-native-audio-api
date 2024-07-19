#ifndef ANDROID
#include "AudioContextWrapper.h"

namespace audiocontext {

    jsi::Value AudioContextWrapper::createOscillator(jsi::Runtime &runtime, const jsi::PropNameID &propNameId) {
        return jsi::Function::createFromHostFunction(runtime, propNameId, 0,
				[](jsi::Runtime& runtime, const jsi::Value& thisVal, const jsi::Value* args, size_t count) -> jsi::Value {
                    auto wrapper = std::make_shared<audiocontext::OscillatorNodeWrapper>();

					auto cppOscillatorNodeHostObjectPtr = std::make_shared<audiocontext::OscillatorNodeHostObject>(wrapper);

					const auto& jsiOscillatorNodeHostObject = jsi::Object::createFromHostObject(runtime, cppOscillatorNodeHostObjectPtr);

					return jsi::Value(runtime, jsiOscillatorNodeHostObject);
			});
    }

    jsi::Value AudioContextWrapper::getDestination(jsi::Runtime& runtime, const jsi::PropNameID& propNameId) {
        throw std::runtime_error("[AudioContextHostObject::getDestination] Not yet implemented!");
    }
} // namespace audiocontext
#endif