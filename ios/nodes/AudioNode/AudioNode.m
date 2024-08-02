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
    NSMutableArray<OscillatorNode *> *connectedOscillators = _context.connectedOscillators;
    for (OscillatorNode *osNode in connectedOscillators) {
        if ([self findNodesToDeprocessHelper:osNode]) {
            [node deprocess:osNode.buffer playerNode:osNode.playerNode nodeToDeprocess:node];
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

- (int)getNumberOfInputs {
    return _numberOfInputs;
}

- (int)getNumberOfOutputs {
    return _numberOfOutputs;
}

@end
