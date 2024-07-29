#import <GainNode.h>
#import "AudioContext.h"

@implementation GainNode

- (instancetype)init:(AudioContext *)context {
    if (self = [super init:context]) {
        self.gain = 0.5;
    }

    return self;
}

- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode {
    playerNode.volume = self.gain;

    [super process:buffer playerNode:playerNode];
}

- (void)deprocess:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode nodeToDeprocess:(AudioNode *)node {
    if (node == self) {
        playerNode.volume = 0.5;
        
        // Deprocess all nodes connected to the disconnected node
        for (AudioNode *cn in self.connectedNodes) {
            [cn deprocess:buffer playerNode:playerNode nodeToDeprocess:cn];
        }
    } else {
        // Continue searching for disconnected node
        [super deprocess:buffer playerNode:playerNode nodeToDeprocess:node];
    }
}

- (void)changeGain:(float)gain {
    self.gain = gain;
    [self.context processNodes];
}

- (float)getGain {
    return self.gain;
}

@end
