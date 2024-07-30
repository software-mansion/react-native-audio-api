#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioParam.h>
#else // when compiled as C++
typedef struct objc_object AudioParam;
#endif // __OBJC__

namespace audiocontext {
    class IOSAudioParam {
        public:
            void setValue(float value);
            float getValue();
            AudioParam *audioParam;
    };
} // namespace audiocontext
