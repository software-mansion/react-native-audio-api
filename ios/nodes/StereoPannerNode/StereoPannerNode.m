#import "StereoPannerNode.h"
#import "AudioContext.h"

// https://webaudio.github.io/web-audio-api/#stereopanner-algorithm

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
    double pan = [_panParam getValueAtTime:[self.context getCurrentTime]];
    double x = (pan + 1) / 2;
    
    double gainL = cos(x * M_PI / 2);
    double gainR = sin(x * M_PI / 2);
    
    parameters.leftGain *= gainL;
    parameters.rightGain *= gainR;
    
    [super processWithParameters:parameters];
}

@end
