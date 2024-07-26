#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <GainNode.h>
#else // when compiled as C++
typedef struct objc_object GainNode;
#endif // __OBJC__

#include "IOSAudioNode.h"
#include "IOSAudioContext.h"

namespace audiocontext {
	class IOSGainNode : public IOSAudioNode {
		public:
			explicit IOSGainNode(std::shared_ptr<IOSAudioContext> context);
            void changeGain(const double gain) const;
            double getGain() const;
        
        protected:
            GainNode *gainNode_;
	};
} // namespace audiocontext
