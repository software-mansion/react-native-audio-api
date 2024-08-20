#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <OscillatorNode.h>
#else // when compiled as C++
typedef struct objc_object OscillatorNode;
#endif // __OBJC__

#include <string>
#include "IOSAudioParam.h"
#include "IOSAudioNode.h"
#include "IOSAudioContext.h"

namespace audiocontext {
    class IOSOscillator : public IOSAudioNode {
        public:
            explicit IOSOscillator(std::shared_ptr<IOSAudioContext> context);
            ~IOSOscillator();
            void start(double time) const;
            void stop(double time) const;
            void setType(const std::string &type) const;
            std::string getType() const;
            std::shared_ptr<IOSAudioParam> getFrequencyParam();
            std::shared_ptr<IOSAudioParam> getDetuneParam();

        protected:
            OscillatorNode *oscillatorNode_;
	};
} // namespace audiocontext
