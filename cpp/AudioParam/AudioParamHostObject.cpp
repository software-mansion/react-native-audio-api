#include "AudioParamHostObject.h"

namespace audiocontext {
    using namespace facebook;

    std::vector<jsi::PropNameID> AudioParamHostObject::getPropertyNames(jsi::Runtime& runtime) {
        std::vector<jsi::PropNameID> propertyNames;
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "value"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "defaultValue"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "minValue"));
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "maxValue"));
        return propertyNames;
    }

    jsi::Value AudioParamHostObject::get(jsi::Runtime& runtime, const jsi::PropNameID& propNameId) {
        auto propName = propNameId.utf8(runtime);

        if (propName == "value") {
            return jsi::Value(wrapper_->getValue());
        }
        
        if (propName == "defaultValue") {
            return jsi::Value(wrapper_->getDefaultValue());
        }

        if (propName == "minValue") {
            return jsi::Value(wrapper_->getMinValue());
        }

        if (propName == "maxValue") {
            return jsi::Value(wrapper_->getMaxValue());
        }

        if(propName == "defaultValue") {
            return jsi::Value(wrapper_->getDefaultValue());
        }

        if(propName == "minValue") {
            return jsi::Value(wrapper_->getMinValue());
        }

        if(propName == "maxValue") {
            return jsi::Value(wrapper_->getMaxValue());
        }

        throw std::runtime_error("Not yet implemented!");
    }

    void AudioParamHostObject::set(jsi::Runtime& runtime, const jsi::PropNameID& propNameId, const jsi::Value& value) {
        auto propName = propNameId.utf8(runtime);

        if(propName == "value") {
            double paramValue = value.getNumber();
            wrapper_->setValue(paramValue);
            return;
        }

        throw std::runtime_error("Not yet implemented!");
    }

}
