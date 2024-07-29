#import "StereoPannerNode.h"
#import "AudioContext.h"

@implementation StereoPannerNode

- (instancetype)init:(AudioContext *)context {
    if (self = [super init:context]) {
        _pan = 0;
    }

    return self;
}

- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode {
    playerNode.pan = _pan;

    [super process:buffer playerNode:playerNode];
}

- (void)deprocess:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode nodeToDeprocess:(AudioNode *)node {
    if (node == self) {
        playerNode.pan = 0;
        
        // Deprocess all nodes connected to the disconnected node
        for (AudioNode *cn in self.connectedNodes) {
            [cn deprocess:buffer playerNode:playerNode nodeToDeprocess:cn];
        }
    } else {
        // Continue searching for disconnected node
        [super deprocess:buffer playerNode:playerNode nodeToDeprocess:node];
    }
}

- (void)setPan:(float)pan {
    _pan = pan;
    [self.context processNodes];
}

- (float)getPan {
    return _pan;
}

@end
