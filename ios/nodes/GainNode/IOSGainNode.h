#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <GainNode.h>
#else // when compiled as C++
typedef struct objc_object GainNode;
#endif // __OBJC__

#include "IOSAudioParam.h"
#include "IOSAudioNode.h"
#include "IOSAudioContext.h"

namespace audiocontext {
    class IOSGainNode : public IOSAudioNode {
        public:
            explicit IOSGainNode(std::shared_ptr<IOSAudioContext> context);
            std::shared_ptr<IOSAudioParam> getAudioParam();

        protected:
            GainNode *gainNode_;
    };
} // namespace audiocontext
