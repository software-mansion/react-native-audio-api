#import "StereoPannerNode.h"
#import "AudioContext.h"

@implementation StereoPannerNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super initWithContext:context]) {
        _audioParam = [[AudioParam alloc] initWithContext:context value:0 minValue:-1 maxValue:1];
        self.numberOfInputs = 1;
        self.numberOfOutputs = 1;
    }

    return self;
}

- (void)clean {
    _audioParam = nil;
}

- (void)process:(float *)buffer frameCount:(AVAudioFrameCount)frameCount {
    // TODO: Implement Stereo Panner compatible with AVAudioSourceNode.

    [super process:buffer frameCount:frameCount];
}

@end
