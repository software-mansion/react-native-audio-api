#import "AudioScheduledSourceNode.h"

@implementation AudioScheduledSourceNode

- (void)start {
    NSLog(@"Attempting to call `start` on a base class where it is not implemented. You must override `start` in a subclass.");
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

- (void)stop {
    NSLog(@"Attempting to call `stop` on a base class where it is not implemented. You must override `stop` in a subclass.");
    @throw [NSException exceptionWithName:NSInternalInconsistencyException
                                   reason:[NSString stringWithFormat:@"You must override %@ in a subclass", NSStringFromSelector(_cmd)]
                                 userInfo:nil];
}

@end

