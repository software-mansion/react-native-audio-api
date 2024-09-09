#pragma once

typedef NS_ENUM(NSInteger, ChannelCountModeEnum) { ChannelCountModeMax, ChannelCountModeClampedMax, ChannelCountModeExplicit };

@interface ChannelCountMode : NSObject

+ (ChannelCountModeEnum)channelCountModeFromString:(NSString *)type;
+ (NSString *)stringFromChannelCountMode:(ChannelCountModeEnum)mode;

@end

