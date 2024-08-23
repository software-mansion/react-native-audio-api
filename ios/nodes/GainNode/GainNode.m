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

- (void)processWithParameters:(PlaybackParameters *)parameters {
    float currentTime = [self.context getCurrentTime];
    parameters.leftGain *= [_gainParam getValueAtTime:currentTime];
    parameters.rightGain *= [_gainParam getValueAtTime:currentTime];

    [super processWithParameters:parameters];
}

@end
