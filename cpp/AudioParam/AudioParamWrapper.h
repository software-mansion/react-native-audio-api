#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioParam.h"
#else
#include "IOSAudioParam.h"
#endif

namespace audiocontext {
    class AudioParamWrapper {
#ifdef ANDROID
        protected:
            std::shared_ptr<AudioParam> param_;
        public:
            explicit AudioParamWrapper(const std::shared_ptr<AudioParam> &param) : param_(param) {}
#else
        protected:
            std::shared_ptr<IOSAudioParam> param_;
        public:
            explicit AudioParamWrapper(std::shared_ptr<IOSAudioParam> param) {
                param_ = param;
            }
#endif
        public:
            double getValue();
            void setValue(double value);
    };
} // namespace audiocontext
