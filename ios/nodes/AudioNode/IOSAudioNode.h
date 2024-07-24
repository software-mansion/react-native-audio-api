#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioNode.h>
#else // when compiled as C++
typedef struct objc_object AudioNode;
#endif // __OBJC__

#import <memory>

namespace audiocontext {
    class IOSAudioNode {
        public:
            AudioNode *AudioNode_;
            void connect(std::shared_ptr<IOSAudioNode> node);
            void disconnect();
    };
} // namespace audiocontext
