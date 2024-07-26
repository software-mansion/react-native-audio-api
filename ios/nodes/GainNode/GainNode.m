#import <GainNode.h>

@implementation GainNode

- (instancetype)init:(AudioContext *)context {
    if (self = [super init:context]) {
        self.gain = 0.5;
        self.playerNodes = [NSMutableArray array];
    }

    return self;
}

- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode {
    playerNode.volume = self.gain;
}

- (void)changeGain:(float)gain {
    self.gain = gain;
    
    for (AVAudioPlayerNode *node in self.playerNodes) {
        node.volume = gain;
    }
}

- (float)getGain {
    return self.gain;
}

- (void)syncPlayerNode:(AVAudioPlayerNode *)node {
    [self.playerNodes addObject:node];
}

- (void)clearPlayerNode:(AVAudioPlayerNode *)node {
    node.volume = 0.5;
    
    NSUInteger index = [self.playerNodes indexOfObject:node];
    if (index != NSNotFound) {
        [self.playerNodes removeObjectAtIndex:index];
    }
}

@end
