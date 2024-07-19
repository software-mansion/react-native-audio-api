#import "AudioNode.h"

@implementation AudioNode

- (instancetype)init {
    if (self = [super init]) {
        _connectedNodes = [NSMutableArray array];
    }

    return self;
}

- (void)connect:(AudioNode *)node {
    if (self.numberOfOutputs > 0) {
        [self.connectedNodes addObject:node];
    }
}

- (void)disconnect {
    [self.connectedNodes removeAllObjects];
}

- (void)process {
    for (AudioNode *node in self.connectedNodes) {
        [node process];
    }
}

@end
