#import <GainNode.h>

@implementation GainNode

- (instancetype)init {
    if (self = [super init]) {
        self.gain = 0.5;
    }

    return self;
}

- (void)process:(AVAudioPCMBuffer *)buffer engine:(AVAudioEngine *)engine {
    engine.mainMixerNode.outputVolume = self.gain;
}

- (void)changeGain:(double)gain {
    self.gain = gain;
}

@end
