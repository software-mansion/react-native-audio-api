#pragma once

#include <jsi/jsi.h>
#include "AudioNodeWrapper.h"

namespace audiocontext {
    using namespace facebook;

    class AudioNodeWrapper;

    class AudioNodeHostObject : public jsi::HostObject {

    protected:
        std::shared_ptr<AudioNodeWrapper> wrapper_;

    public:
        explicit AudioNodeHostObject(const std::shared_ptr<AudioNodeWrapper> &wrapper) : wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;
    };
} // namespace audiocontext
