#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <OscillatorNode.h>
#else // when compiled as C++
typedef struct objc_object OscillatorNode;
#endif // __OBJC__

#include <string>

#include "IOSAudioNode.h"

namespace audiocontext {
	class IOSOscillator : public IOSAudioNode {
		public:
			explicit IOSOscillator();
			void start() const;
			void stop() const;
			void changeFrequency(const float frequency) const;
			float getFrequency() const;
			void changeDetune(const float detune) const;
			float getDetune() const;
			void setType(const std::string &type) const;
			std::string getType() const;

		protected:
			OscillatorNode *OscillatorNode_;
	};
} // namespace audiocontext
