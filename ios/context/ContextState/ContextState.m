#import "ContextState.h"

@implementation ContextState

+ (ContextStateEnum)fromString:(NSString *)state {
    NSString *lowercaseState = [state lowercaseString];
    if ([lowercaseState isEqualToString:@"running"]) {
        return ContextStateRunning;
    } else if ([lowercaseState isEqualToString:@"closed"]) {
        return ContextStateClosed;
    } else {
        @throw [NSException exceptionWithName:NSInvalidArgumentException
                                       reason:[NSString stringWithFormat:@"Unknown context state: %@", state]
                                     userInfo:nil];
    }
}

+ (NSString *)toString:(ContextStateEnum)state {
    switch (state) {
        case ContextStateRunning:
            return @"running";
        case ContextStateClosed:
            return @"closed";
    }
}

@end
