#pragma once

#include <jsi/jsi.h>
#include "AudioNodeHostObject.h"
#include "GainNodeWrapper.h"

namespace audiocontext {
    using namespace facebook;

    class GainNodeWrapper;

    class GainNodeHostObject : public AudioNodeHostObject {
    private:
        std::shared_ptr<GainNodeWrapper> wrapper_;

    public:
        explicit GainNodeHostObject(std::shared_ptr<GainNodeWrapper> wrapper) : AudioNodeHostObject(wrapper), wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;

        static std::shared_ptr<GainNodeHostObject> createFromWrapper(std::shared_ptr<GainNodeWrapper> wrapper) {
            return std::make_shared<GainNodeHostObject>(wrapper);
        }

        std::shared_ptr<GainNodeWrapper> getWrapper() {
            return wrapper_;
        }
    };
} // namespace audiocontext
