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

- (void)syncPlayerNode:(AVAudioPlayerNode *)node {
    NSLog(@"Attempting to call `syncPlayerNode` on a base class where it is not implemented. You must override `syncPlayerNode` in a subclass.");
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

- (void)clearPlayerNode:(AVAudioPlayerNode *)node {
    NSLog(@"Attempting to call `clearPlayerNode` on a base class where it is not implemented. You must override `clearPlayerNode` in a subclass.");
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

@end
