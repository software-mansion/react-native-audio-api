#include "AudioDestinationNodeHostObject.h"

namespace audiocontext {
    using namespace facebook;

    std::vector<jsi::PropNameID> AudioDestinationNodeHostObject::getPropertyNames(jsi::Runtime& runtime) {
        std::vector<jsi::PropNameID> propertyNames;
        return propertyNames;
    }

    jsi::Value AudioDestinationNodeHostObject::get(jsi::Runtime& runtime, const jsi::PropNameID& propNameId) {
        auto propName = propNameId.utf8(runtime);

        throw std::runtime_error("Not yet implemented!");
    }

    void AudioDestinationNodeHostObject::set(jsi::Runtime& runtime, const jsi::PropNameID& propNameId, const jsi::Value& value) {
        auto propName = propNameId.utf8(runtime);

        throw std::runtime_error("Not yet implemented!");
    }

}
