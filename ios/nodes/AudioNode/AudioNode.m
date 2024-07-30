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
    for (AudioNode *node in _connectedNodes) {
        [node process:buffer playerNode:playerNode];
    }
}

- (void)deprocess:(AVAudioPCMBuffer *)buffer playerNode:(AVAudioPlayerNode *)playerNode nodeToDeprocess:(AudioNode *)node {
    for (AudioNode *node in _connectedNodes) {
        [node deprocess:buffer playerNode:playerNode nodeToDeprocess:node];
    }
}

- (void)connect:(AudioNode *)node {
    if (_numberOfOutputs > 0) {
        [_connectedNodes addObject:node];
    }
}

- (void)disconnect:(AudioNode *)node {
    NSUInteger index = [_connectedNodes indexOfObject:node];
    if (index != NSNotFound) {
        [self findNodesToDeprocess:node];
        [_connectedNodes removeObjectAtIndex:index];
    }
}

- (void)findNodesToDeprocess:(AudioNode *)node {
    NSMutableArray<OscillatorNode *> *connectedNodes = _context.connectedOscillators;
    for (OscillatorNode *cn in connectedNodes) {
        if ([self findNodesToDeprocessHelper:cn]) {
            [node deprocess:cn.buffer playerNode:cn.playerNode nodeToDeprocess:node];
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
