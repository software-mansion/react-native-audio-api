#pragma once

#include <memory>

#ifdef ANDROID
#include "AudioParam.h"
#endif

namespace audiocontext {
    using namespace facebook;

    class AudioParamWrapper {
#ifdef ANDROID
    protected:
        std::shared_ptr<AudioParam> param_;
    public:
        explicit AudioParamWrapper(const std::shared_ptr<AudioParam> &param);
#else
        public:
        explicit AudioParamWrapper() {}
#endif
    private:
        double defaultValue_;
        double minValue_;
        double maxValue_;
    public:
        double getValue();
        void setValue(double value);
        double getDefaultValue() const;
        double getMinValue() const;
        double getMaxValue() const;
        void setValueAtTime(double value, double startTime);
        void linearRampToValueAtTime(double value, double endTime);
    };
} // namespace audiocontext
