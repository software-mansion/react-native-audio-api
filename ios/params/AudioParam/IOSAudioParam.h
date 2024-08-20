#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioParam.h>
#else // when compiled as C++
typedef struct objc_object AudioParam;
#endif // __OBJC__

namespace audiocontext {
    class IOSAudioParam {
        public:
            ~IOSAudioParam();
            void setValue(float value);
            float getValue();
            float getMinValue();
            float getMaxValue();
            float getDefaultValue();
            AudioParam *audioParam;
            void setValueAtTime(float value, double time);
            void linearRampToValueAtTime(float value, double time);
            void exponentialRampToValueAtTime(float value, double time);

    };
} // namespace audiocontext
