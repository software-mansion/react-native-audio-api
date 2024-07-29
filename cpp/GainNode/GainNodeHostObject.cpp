#include "GainNodeHostObject.h"

namespace audiocontext
{
    using namespace facebook;

    std::vector<jsi::PropNameID> GainNodeHostObject::getPropertyNames(jsi::Runtime &runtime)
    {
        std::vector<jsi::PropNameID> propertyNames = AudioNodeHostObject::getPropertyNames(runtime);
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "gain"));
        return propertyNames;
    }

    jsi::Value GainNodeHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
    {
        auto propName = propNameId.utf8(runtime);

        if (propName == "gain")
        {
            auto gain = wrapper_->getGain();
            return jsi::Value(gain);
        }

        return AudioNodeHostObject::get(runtime, propNameId);
    }

    void GainNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
    {
        auto propName = propNameId.utf8(runtime);

        if (propName == "gain")
        {
            double gain = value.getNumber();
            wrapper_->setGain(gain);
            return;
        }

        return AudioNodeHostObject::set(runtime, propNameId, value);
    }
}
