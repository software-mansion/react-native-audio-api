#pragma once

#include <jsi/jsi.h>

#include <utility>
#include "AudioContextWrapper.h"
#include "OscillatorNodeHostObject.h"
#include "AudioDestinationNodeHostObject.h"
#include "GainNodeHostObject.h"

namespace audiocontext
{
    using namespace facebook;

    class AudioContextWrapper;

    class AudioContextHostObject : public jsi::HostObject
    {
    private:
        std::shared_ptr<AudioContextWrapper> wrapper_;

    public:
        explicit AudioContextHostObject(std::shared_ptr<AudioContextWrapper> wrapper) : wrapper_(wrapper) {}
#ifdef ANDROID
        static void createAndInstallFromWrapper(const std::shared_ptr<AudioContextWrapper>& wrapper, jlong jsContext) {
            auto runtime = reinterpret_cast<jsi::Runtime *>(jsContext);
            auto hostObject = std::make_shared<AudioContextHostObject>(wrapper);
            auto object = jsi::Object::createFromHostObject(*runtime, hostObject);
            runtime->global().setProperty(*runtime, "__AudioContext", std::move(object));
        }
#endif
        jsi::Value get(jsi::Runtime &runtime, const jsi::PropNameID &name) override;
        void set(jsi::Runtime &runtime, const jsi::PropNameID &name, const jsi::Value &value) override;
        std::vector<jsi::PropNameID> getPropertyNames(jsi::Runtime &rt) override;
  };
} // namespace audiocontext
