#pragma once

#include <jsi/jsi.h>
#include "StereoPannerNodeWrapper.h"
#include "AudioNodeHostObject.h"

namespace audiocontext {
    using namespace facebook;

    class StereoPannerNodeWrapper;

    class StereoPannerNodeHostObject : public AudioNodeHostObject {
    protected:
        std::shared_ptr<StereoPannerNodeWrapper> wrapper_;

    public:
        explicit StereoPannerNodeHostObject(const std::shared_ptr<StereoPannerNodeWrapper> &wrapper) : AudioNodeHostObject(wrapper), wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;

        static std::shared_ptr<StereoPannerNodeHostObject> createFromWrapper(const std::shared_ptr<StereoPannerNodeWrapper> &wrapper) {
            return std::make_shared<StereoPannerNodeHostObject>(wrapper);
        }
    };
} // namespace audiocontext
