#pragma once

typedef NS_ENUM(NSInteger, ContextStateEnum) {
    ContextStateRunning,
    ContextStateClosed
};

@interface ContextState : NSObject

+ (ContextStateEnum)fromString:(NSString *)state;
+ (NSString *)toString:(ContextStateEnum)state;

@end
