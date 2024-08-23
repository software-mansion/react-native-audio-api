#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioDestinationNode.h>
#else  // when compiled as C++
typedef struct objc_object AudioDestinationNode;
#endif // __OBJC__

#include "IOSAudioNode.h"
#include "IOSAudioContext.h"

#import <memory>

namespace audiocontext {
    class IOSAudioDestinationNode : public IOSAudioNode {
        public:
            explicit IOSAudioDestinationNode(std::shared_ptr<IOSAudioContext> context);
            ~IOSAudioDestinationNode();

        protected:
        AudioDestinationNode *destination_;
    };
} // namespace audiocontext

