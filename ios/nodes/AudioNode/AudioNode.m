#import "AudioNode.h"

@implementation AudioNode

- (instancetype)init {
    if (self = [super init]) {
        _connectedNodes = [NSMutableDictionary dictionary];
        _uuid = [NSUUID UUID];
    }

    return self;
}

- (void)connect:(AudioNode *)node {
    if (self.numberOfOutputs > 0) {
        self.connectedNodes[node.uuid] = node;
    }
}

- (void)disconnect:(AudioNode *)node {
    [self.connectedNodes removeObjectForKey:node.uuid];
}

- (void)disconnectAttachedNode:(AudioNode *)node {
    
}

- (void)process:(AVAudioPCMBuffer *)buffer engine:(AVAudioEngine *)engine uuid:(NSUUID *)uuid {
    for (AudioNode *node in [self.connectedNodes allValues]) {
        [node process:buffer engine:engine uuid:uuid];
    }
}

@end
