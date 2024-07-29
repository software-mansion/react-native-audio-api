#pragma once

#include <jsi/jsi.h>
#include "OscillatorNodeWrapper.h"
#include "AudioDestinationNodeHostObject.h"
#include "GainNodeHostObject.h"
#include "AudioNodeHostObject.h"

namespace audiocontext {
    using namespace facebook;

    class OscillatorNodeWrapper;

    class OscillatorNodeHostObject : public AudioNodeHostObject {
    private:
        std::shared_ptr<OscillatorNodeWrapper> wrapper_;

    public:
        explicit OscillatorNodeHostObject(const std::shared_ptr<OscillatorNodeWrapper> &wrapper) : AudioNodeHostObject(wrapper), wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;

        static std::shared_ptr<OscillatorNodeHostObject> createFromWrapper(const std::shared_ptr<OscillatorNodeWrapper> &wrapper) {
            return std::make_shared<OscillatorNodeHostObject>(wrapper);
        }
    };
} // namespace audiocontext
