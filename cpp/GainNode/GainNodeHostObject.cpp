#include "GainNodeHostObject.h"

namespace audiocontext
{
    using namespace facebook;

    GainNodeHostObject::GainNodeHostObject(const std::shared_ptr<GainNodeWrapper> &wrapper) : AudioNodeHostObject(wrapper), wrapper_(wrapper) {
        auto gainParamWrapper = wrapper->getGainParam();
        gainParam_ = AudioParamHostObject::createFromWrapper(gainParamWrapper);
    }

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
            return jsi::Object::createFromHostObject(runtime, gainParam_);
        }

        return AudioNodeHostObject::get(runtime, propNameId);
    }

    void GainNodeHostObject::set(jsi::Runtime &runtime, const jsi::PropNameID &propNameId, const jsi::Value &value)
    {
        auto propName = propNameId.utf8(runtime);

        return AudioNodeHostObject::set(runtime, propNameId, value);
    }
}
