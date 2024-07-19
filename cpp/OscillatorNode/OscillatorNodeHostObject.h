#pragma once

#include <jsi/jsi.h>
#include "OscillatorNodeWrapper.h"

namespace audiocontext {
    using namespace facebook;

    class OscillatorNodeWrapper;

    class OscillatorNodeHostObject : public jsi::HostObject {
    private:
        std::shared_ptr<OscillatorNodeWrapper> wrapper_;

    public:
        explicit OscillatorNodeHostObject(std::shared_ptr<OscillatorNodeWrapper> wrapper) : wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;
    };
} // namespace audiocontext
