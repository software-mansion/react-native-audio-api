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

- (void)process:(float *)buffer frameCount:(AVAudioFrameCount)frameCount {
    // Do nothing
}

@end
