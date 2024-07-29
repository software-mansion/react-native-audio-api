#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import "StereoPannerNode.h"
#else // when compiled as C++
typedef struct objc_object StereoPannerNode;
#endif // __OBJC__

#include "IOSAudioNode.h"
#include "IOSAudioContext.h"

namespace audiocontext {
    class IOSStereoPannerNode : public IOSAudioNode {
        public:
            explicit IOSStereoPannerNode(std::shared_ptr<IOSAudioContext> context);
            void setPan(const float pan) const;
            float getPan() const;

        protected:
            StereoPannerNode *panner_;
    };
} // namespace audiocontext
