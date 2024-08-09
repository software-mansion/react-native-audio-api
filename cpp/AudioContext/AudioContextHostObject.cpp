#include "AudioContextHostObject.h"

namespace audiocontext {
    using namespace facebook;

    AudioContextHostObject::AudioContextHostObject(const std::shared_ptr<AudioContextWrapper>& wrapper) : wrapper_(wrapper) {
        auto destinationNodeWrapper = wrapper_->getDestination();
        destinationNode_ = AudioDestinationNodeHostObject::createFromWrapper(destinationNodeWrapper);
    }

    std::vector<jsi::PropNameID> AudioContextHostObject::getPropertyNames(jsi::Runtime& runtime) {
        std::vector<jsi::PropNameID> propertyNames;
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "destination"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "state"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "sampleRate"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "currentTime"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createOscillator"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createGain"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createStereoPanner"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "createBiquadFilter"));
        propertyNames.push_back(jsi::PropNameID::forUtf8(runtime, "close"));
        return propertyNames;
    }

    jsi::Value AudioContextHostObject::get(jsi::Runtime& runtime, const jsi::PropNameID& propNameId) {
        auto propName = propNameId.utf8(runtime);

        if (propName == "destination") {
            return jsi::Object::createFromHostObject(runtime, destinationNode_);
        }

        if (propName == "state") {
            return jsi::String::createFromUtf8(runtime, wrapper_->getState());
        }

        if (propName == "sampleRate") {
            return jsi::Value(wrapper_->getSampleRate());
        }

        if (propName == "currentTime") {
            return jsi::Value(wrapper_->getCurrentTime());
        }

        if (propName == "createOscillator") {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) -> jsi::Value {
                auto oscillator = wrapper_->createOscillator();
                auto oscillatorHostObject = OscillatorNodeHostObject::createFromWrapper(oscillator);
                return jsi::Object::createFromHostObject(runtime, oscillatorHostObject);
            });
        }

        if (propName == "createGain") {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) -> jsi::Value {
                auto gain = wrapper_->createGain();
                auto gainHostObject = GainNodeHostObject::createFromWrapper(gain);
                return jsi::Object::createFromHostObject(runtime, gainHostObject);
            });
        }

        if (propName == "createStereoPanner") {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) -> jsi::Value {
                auto stereoPanner = wrapper_->createStereoPanner();
                auto stereoPannerHostObject = StereoPannerNodeHostObject::createFromWrapper(stereoPanner);
                return jsi::Object::createFromHostObject(runtime, stereoPannerHostObject);
            });
        }

        if (propName == "createBiquadFilter") {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) -> jsi::Value {
                auto biquadFilter = wrapper_->createBiquadFilter();
                auto biquadFilterHostObject = BiquadFilterNodeHostObject::createFromWrapper(biquadFilter);
                return jsi::Object::createFromHostObject(runtime, biquadFilterHostObject);
            });
        }

        if (propName == "close") {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 0, [this](jsi::Runtime& runtime, const jsi::Value& thisValue, const jsi::Value* arguments, size_t count) -> jsi::Value {
                wrapper_->close();
                return jsi::Value::undefined();
            });
        }

        throw std::runtime_error("Not yet implemented!");
    }

    void AudioContextHostObject::set(jsi::Runtime& runtime, const jsi::PropNameID& propNameId, const jsi::Value& value) {
        auto propName = propNameId.utf8(runtime);

        throw std::runtime_error("Not yet implemented!");
    }
}
