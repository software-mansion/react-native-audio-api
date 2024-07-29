#import "AudioNode.h"
#import "AudioContext.h"

@implementation AudioNode

- (instancetype)init:(AudioContext *)context {
    if (self = [super init]) {
        _connectedNodes = [NSMutableArray array];
        _context = context;
    }

    return self;
}

- (void)process:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode {
    for (AudioNode *node in self.connectedNodes) {
        [node process:buffer playerNode:playerNode];
    }
}

- (void)deprocess:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode nodeToDeprocess:(AudioNode *)node {
    for (AudioNode *node in self.connectedNodes) {
        [node deprocess:buffer playerNode:playerNode nodeToDeprocess:node];
    }
}

- (void)connect:(AudioNode *)node {
    if (self.numberOfOutputs > 0) {
        [self.connectedNodes addObject:node];
    }
}

- (void)disconnect:(AudioNode *)node {
    NSUInteger index = [self.connectedNodes indexOfObject:node];
    if (index != NSNotFound) {
        [self findNodesToDeprocess];
        [self.connectedNodes removeObjectAtIndex:index];
    }
}

- (void)findNodesToDeprocess {
    NSMutableArray<OscillatorNode *> *connectedNodes = self.context.connectedOscillators;
    for (OscillatorNode *node in connectedNodes) {
        if ([self findNodesToDeprocessHelper:node]) {
            [node deprocess:node.buffer playerNode:node.playerNode nodeToDeprocess:self];
        }
    }
}

- (Boolean)findNodesToDeprocessHelper:(AudioNode *)node {
    if (node == self) {
        return true;
    }
    
    for (AudioNode *cn in node.connectedNodes) {
        if ([self findNodesToDeprocessHelper:cn]) {
            return true;
        }
    }
    
    return false;
}

@end
