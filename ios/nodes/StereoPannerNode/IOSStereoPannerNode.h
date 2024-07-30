#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import "StereoPannerNode.h"
#else // when compiled as C++
typedef struct objc_object StereoPannerNode;
#endif // __OBJC__

#include "IOSAudioParam.h"
#include "IOSAudioNode.h"
#include "IOSAudioContext.h"

namespace audiocontext {
    class IOSStereoPannerNode : public IOSAudioNode {
        public:
            explicit IOSStereoPannerNode(std::shared_ptr<IOSAudioContext> context);
            std::shared_ptr<IOSAudioParam> getAudioParam();

        protected:
            StereoPannerNode *panner_;
    };
} // namespace audiocontext
