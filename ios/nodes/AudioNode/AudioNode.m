#import "AudioNode.h"
#import "AudioContext.h"

@implementation AudioNode

- (instancetype)initWithContext:(AudioContext *)context {
    if (self = [super init]) {
        _connectedNodes = [NSMutableArray array];
        _context = context;
    }

    return self;
}

- (void)cleanup {
    _context = nil;
    [_connectedNodes removeAllObjects];
    _connectedNodes = nil;

}

- (void)processWithParameters:(PlaybackParameters *)parameters; {
    for (AudioNode *node in _connectedNodes) {
        [node processWithParameters:parameters];
    }
}

- (void)process:(AVAudioFrameCount)frameCount bufferList:(AudioBufferList *)bufferList; {
    for (AudioNode *node in _connectedNodes) {
        [node process:frameCount bufferList:bufferList];
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
        [_connectedNodes removeObjectAtIndex:index];
    }
}

- (int)getNumberOfInputs {
    return _numberOfInputs;
}

- (int)getNumberOfOutputs {
    return _numberOfOutputs;
}

@end
