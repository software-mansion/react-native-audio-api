#pragma once

#include <jsi/jsi.h>
#include "AudioNodeWrapper.h"

namespace audiocontext {
    using namespace facebook;

    class AudioNodeWrapper;

    class AudioNodeHostObject : public jsi::HostObject {

    public:
        std::shared_ptr<AudioNodeWrapper> wrapper_;
        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;

    public:
        explicit AudioNodeHostObject(std::shared_ptr<AudioNodeWrapper> wrapper) : wrapper_(wrapper) {}
    };
} // namespace audiocontext
