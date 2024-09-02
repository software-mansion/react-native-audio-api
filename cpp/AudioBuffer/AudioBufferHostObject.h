#pragma once

#include <jsi/jsi.h>

#include "AudioBufferWrapper.h"

namespace audiocontext {
    using namespace facebook;

    class AudioBufferWrapper;

    class AudioBufferHostObject : public jsi::HostObject {

    protected:
        std::shared_ptr<AudioBufferWrapper> wrapper_;

    public:
        explicit AudioBufferHostObject(const std::shared_ptr<AudioBufferWrapper> &wrapper) : wrapper_(wrapper) {}

        jsi::Value get(jsi::Runtime& runtime, const jsi::PropNameID& name) override;
        void set(jsi::Runtime& runtime, const jsi::PropNameID& name, const jsi::Value& value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime& rt) override;

        static std::shared_ptr<AudioBufferHostObject> createFromWrapper(const std::shared_ptr<AudioBufferWrapper> &wrapper) {
            return std::make_shared<AudioBufferHostObject>(wrapper);
        }
    };
} // namespace audiocontext
