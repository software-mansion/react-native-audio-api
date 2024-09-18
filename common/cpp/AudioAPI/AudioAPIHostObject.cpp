#include "AudioAPIHostObject.h"

namespace audioapi {
    using namespace facebook;

    std::vector<jsi::PropNameID> AudioAPIHostObject::getPropertyNames(
            jsi::Runtime &runtime) {
        std::vector<jsi::PropNameID> propertyNames;
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createAudioContext"));
        return propertyNames;
    }

    jsi::Value AudioAPIHostObject::get(
            jsi::Runtime &runtime,
            const jsi::PropNameID &propNameId) {
        auto propName = propNameId.utf8(runtime);

        if (propName == "createAudioContext") {
            return jsi::Function::createFromHostFunction(
                    runtime,
                    propNameId,
                    0,
                    [this](
                            jsi::Runtime &runtime,
                            const jsi::Value &thisValue,
                            const jsi::Value *arguments,
                            size_t count) -> jsi::Value {
                        auto audioContext = wrapper_->createAudioContext();
                        auto audioContextHostObject =
                                AudioContextHostObject::createFromWrapper(audioContext);
                        return jsi::Object::createFromHostObject(
                                runtime, audioContextHostObject);
                    });
        }

        throw std::runtime_error("Not yet implemented!");
    }

    void AudioAPIHostObject::set(
            jsi::Runtime &runtime,
            const jsi::PropNameID &propNameId,
            const jsi::Value &value) {
        auto propName = propNameId.utf8(runtime);

        throw std::runtime_error("Not yet implemented!");
    }
} // namespace audioapi
