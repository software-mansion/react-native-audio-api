#import "AudioNode.h"

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

- (void)connect:(AudioNode *)node {
    if (self.numberOfOutputs > 0) {
        [self.connectedNodes addObject:node];
    }
}

- (void)disconnect:(AudioNode *)node {
    NSUInteger index = [self.connectedNodes indexOfObject:node];
    if (index != NSNotFound) {
        [self.connectedNodes removeObjectAtIndex:index];
    }
}

- (void)addConnectedTo:(AVAudioPlayerNode *)node {
    NSLog(@"Attempting to call `addConnectedTo` on a base class where it is not implemented. You must override `addConnectedTo` in a subclass.");
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

- (void)removeConnectedTo:(AVAudioPlayerNode *)node {
    NSLog(@"Attempting to call `removeConnectedTo` on a base class where it is not implemented. You must override `removeConnectedTo` in a subclass.");
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

@end
