#import <GainNode.h>
#import "AudioContext.h"
#import "Constants.h"

@implementation GainNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super initWithContext:context]) {
        _gainParam = [[AudioParam alloc] initWithContext:context value:1 minValue:-[Constants maxGain] maxValue:[Constants maxGain]];
        self.numberOfInputs = 1;
        self.numberOfOutputs = 1;
    }

    return self;
}

- (void)cleanup {
    _gainParam = nil;
}

- (void)process:(float *)buffer frameCount:(AVAudioFrameCount)frameCount {
    for (int frame = 0; frame < frameCount; frame++) {
        buffer[frame] *= [_gainParam getValueAtTime:[self.context getCurrentTime]];
    }

    [super process:buffer frameCount:frameCount];
}

@end
