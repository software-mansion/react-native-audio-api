#pragma once

typedef NS_ENUM(NSInteger, ChannelInterpretationEnum) { ChannelInterpretationSpeakers, ChannelInterpretationDiscrete };

@interface ChannelInterpretation : NSObject

+ (ChannelInterpretationEnum)channelInterpretationFromString:(NSString *)type;
+ (NSString *)stringFromChannelInterpretation:(ChannelInterpretationEnum)channelInterpretation;

@end
