#pragma once

#ifdef __OBJC__ // when compiled as Objective-C++
#import <OscillatorNode.h>
#else // when compiled as C++
typedef struct objc_object OscillatorNode;
#endif // __OBJC__

namespace audiocontext {
	class IOSOscillator {
		public:
			explicit IOSOscillator(const float frequency);
			void start() const;
			void stop() const;
            void changeFrequency(const float frequency) const;

		protected:
			OscillatorNode *OscillatorNode_;
	};
} // namespace audiocontext
