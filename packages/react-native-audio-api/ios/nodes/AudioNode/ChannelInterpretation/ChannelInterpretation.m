#import "ChannelInterpretation.h"

@implementation ChannelInterpretation

+ (ChannelInterpretationEnum)channelInterpretationFromString:(NSString *)type
{
  if ([type isEqualToString:@"speakers"]) {
    return ChannelInterpretationSpeakers;
  }

  if ([type isEqualToString:@"discrete"]) {
    return ChannelInterpretationDiscrete;
  }

  @throw [NSException exceptionWithName:@"Invalid channel interpretation type"
                                 reason:@"You have to pick correct channel interpretation: ['speakers', 'discrete']"
                               userInfo:nil];
}

+ (NSString *)stringFromChannelInterpretation:(ChannelInterpretationEnum)channelInterpretation
{
  switch (channelInterpretation) {
    case ChannelInterpretationSpeakers:
      return @"speakers";
    case ChannelInterpretationDiscrete:
      return @"discrete";
    default:
      @throw [NSException exceptionWithName:@"Invalid channel interpretation type"
                                     reason:@"You have to pick correct channel interpretation: ['speakers', 'discrete']"
                                   userInfo:nil];
  }

  return nil;
}

@end
