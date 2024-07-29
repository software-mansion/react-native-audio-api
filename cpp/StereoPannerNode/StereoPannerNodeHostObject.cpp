#include "StereoPannerNodeHostObject.h"

namespace audiocontext
{
    using namespace facebook;

    std::vector<jsi::PropNameID> StereoPannerNodeHostObject::getPropertyNames(jsi::Runtime &runtime)
    {
        std::vector<jsi::PropNameID> propertyNames = AudioNodeHostObject::getPropertyNames(runtime);
        propertyNames.push_back(jsi::PropNameID::forAscii(runtime, "pan"));
        return propertyNames;
    }

    jsi::Value StereoPannerNodeHostObject::get(jsi::Runtime &runtime, const jsi::PropNameID &propNameId)
    {
        auto propName = propNameId.utf8(runtime);

        if (propName == "pan")
        {
            auto gain = wrapper_->getPan();
            return jsi::Value(gain);
        }

        return AudioNodeHostObject::get(runtime, propNameId);
    }

    void StereoPannerNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
    {
        auto propName = propNameId.utf8(runtime);

        if (propName == "pan")
        {
            double pan = value.getNumber();
            wrapper_->setPan(pan);
            return;
        }

        throw std::runtime_error("Not yet implemented!");
    }
}
