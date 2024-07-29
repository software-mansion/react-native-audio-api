#pragma once

#include <jsi/jsi.h>
#include "GainNodeWrapper.h"
#include "AudioNodeHostObject.h"

namespace audiocontext {
    using namespace facebook;

    class GainNodeWrapper;

    class GainNodeHostObject : public AudioNodeHostObject {
    protected:
        std::shared_ptr<GainNodeWrapper> wrapper_;

    public:
        explicit GainNodeHostObject(const std::shared_ptr<GainNodeWrapper> &wrapper) : AudioNodeHostObject(wrapper), wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;

        static std::shared_ptr<GainNodeHostObject> createFromWrapper(const std::shared_ptr<GainNodeWrapper> &wrapper) {
            return std::make_shared<GainNodeHostObject>(wrapper);
        }
    };
} // namespace audiocontext
