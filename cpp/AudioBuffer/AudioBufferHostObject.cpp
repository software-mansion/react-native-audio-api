#include "AudioBufferHostObject.h"

namespace audiocontext {
    using namespace facebook;

    std::vector<jsi::PropNameID> AudioBufferHostObject::getPropertyNames(jsi::Runtime& runtime) {
        std::vector<jsi::PropNameID> propertyNames;
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "buffer"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "sampleRate"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "length"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "duration"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "numberOfChannels"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "getChannelData"));
        return propertyNames;
    }

    jsi::Value AudioBufferHostObject::get(jsi::Runtime& runtime, const jsi::PropNameID& propNameId) {
        auto propName = propNameId.utf8(runtime);

        if (propName == "sampleRate") {
            return jsi::Value(wrapper_->getSampleRate());
        }

        if (propName == "length") {
            return jsi::Value(wrapper_->getLength());
        }

        if (propName == "duration") {
            return jsi::Value(wrapper_->getDuration());
        }

        if (propName == "numberOfChannels") {
            return jsi::Value(wrapper_->getNumberOfChannels());
        }

        if(propName == "getChannelData") {
            return jsi::Function::createFromHostFunction(runtime, propNameId, 1, [this](jsi::Runtime& rt, const jsi::Value& thisVal, const jsi::Value* args, size_t count) -> jsi::Value {
                int channel = args[0].getNumber();
                short** channelData = wrapper_->getChannelData(channel);
                return jsi::Value::null();
            });
        }

        throw std::runtime_error("Not yet implemented!");
    }

    void AudioBufferHostObject::set(jsi::Runtime& runtime, const jsi::PropNameID& propNameId, const jsi::Value& value) {
        auto propName = propNameId.utf8(runtime);

        throw std::runtime_error("Not yet implemented!");
    }

}
