#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioNode.h>
#import <AVFoundation/AVFoundation.h>
#else // when compiled as C++
typedef struct objc_object AudioNode;
typedef struct objc_object AVAudioPlayerNode;
#endif // __OBJC__

#import <memory>

namespace audiocontext {
    class IOSAudioNode {
        public:
            AudioNode *audioNode_;
            virtual void connect(std::shared_ptr<IOSAudioNode> node);
            virtual void disconnect(std::shared_ptr<IOSAudioNode> node);
            virtual void syncPlayerNode(AVAudioPlayerNode *node);
            virtual void clearPlayerNode(AVAudioPlayerNode *node);
    };
} // namespace audiocontext
