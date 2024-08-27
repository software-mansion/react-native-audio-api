#import <AudioDestinationNode.h>
#import "AudioContext.h"

@implementation AudioDestinationNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super initWithContext:context]) {
        self.numberOfInputs = INT_MAX;
        self.numberOfOutputs = 0;
    }

    return self;
}

- (void)processWithParameters:(PlaybackParameters *)parameters {
    for (int frame = 0; frame < parameters.frameCount; frame++) {
        parameters.leftBuffer[frame] *= parameters.leftGain;
        parameters.rightBuffer[frame] *= parameters.rightGain;
    }
}

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList {
  // do nothing
}

@end
