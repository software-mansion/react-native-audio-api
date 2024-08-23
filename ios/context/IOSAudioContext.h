#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <AudioContext.h>
#else // when compiled as C++
typedef struct objc_object AudioContext;
#endif // __OBJC__

#include <string>
#include "IOSAudioContext.h"

namespace audiocontext {
	class IOSAudioContext {
		public:
            explicit IOSAudioContext();
            ~IOSAudioContext();
            void close();
            double getCurrentTime();
            std::string getState();
            double getSampleRate();
            AudioContext *audioContext_;
	};
} // namespace audiocontext
