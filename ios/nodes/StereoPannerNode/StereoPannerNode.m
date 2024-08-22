#import "StereoPannerNode.h"
#import "AudioContext.h"

@implementation StereoPannerNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super initWithContext:context]) {
        _panParam = [[AudioParam alloc] initWithContext:context value:0 minValue:-1 maxValue:1];
        self.numberOfInputs = 1;
        self.numberOfOutputs = 1;
    }

    return self;
}

- (void)cleanup {
    _panParam = nil;
}

- (void)processWithParameters:(PlaybackParameters *)parameters {
    double currentTime = [self.context getCurrentTime];
    parameters.leftGain *= fmin(1.0 - [_panParam getValueAtTime:currentTime], 1.0);
    parameters.rightGain *= fmin(1.0 + [_panParam getValueAtTime:currentTime], 1.0);

    [super processWithParameters:parameters];
}

@end
