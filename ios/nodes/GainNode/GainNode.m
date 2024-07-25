#import <GainNode.h>

@implementation GainNode

- (instancetype)init {
    if (self = [super init]) {
        self.gain = 0.5;
        self.connectedEngines = [NSMutableDictionary dictionary];
    }

    return self;
}

- (void)process:(AVAudioPCMBuffer *)buffer engine:(AVAudioEngine *)engine uuid:(NSUUID *)uuid {
    if (!self.connectedEngines[uuid]) {
        self.connectedEngines[uuid] = engine;
    }
    
    engine.mainMixerNode.outputVolume = self.gain;
}

- (void)disconnectAttachedNode:(AudioNode *)node {
    AVAudioEngine *engine = self.connectedEngines[node.uuid];
    engine.mainMixerNode.outputVolume = 0.5;
    [self.connectedEngines removeObjectForKey:node.uuid];
}

- (void)changeGain:(double)gain {
    self.gain = gain;
    
    for (AVAudioEngine *engine in [self.connectedEngines allValues]) {
        engine.mainMixerNode.outputVolume = gain;
    }
}

- (double)getGain {
    return self.gain;
}

@end
