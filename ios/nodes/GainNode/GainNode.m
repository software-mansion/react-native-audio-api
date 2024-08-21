#import <GainNode.h>
#import "AudioContext.h"

@implementation GainNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super initWithContext:context]) {
        _gainParam = [[AudioParam alloc] initWithContext:context value:0.5 minValue:0 maxValue:1];
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
