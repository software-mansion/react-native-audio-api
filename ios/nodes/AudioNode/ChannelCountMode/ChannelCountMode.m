#import "ChannelCountMode.h"

@implementation ChannelCountMode

+ (ChannelCountModeEnum)channelCountModeFromString:(NSString *)mode
{
  if ([mode isEqualToString:@"max"]) {
    return ChannelCountModeMax;
  }

  if ([mode isEqualToString:@"clamped-max"]) {
    return ChannelCountModeClampedMax;
  }

  if ([mode isEqualToString:@"explicit"]) {
    return ChannelCountModeExplicit;
  }

  @throw
      [NSException exceptionWithName:@"Invalid wave type"
                              reason:@"You have to pick correct channel count mode: ['max', 'clamped-max', 'explicit']"
                            userInfo:nil];
}

+ (NSString *)stringFromChannelCountMode:(ChannelCountModeEnum)mode
{
  switch (mode) {
    case ChannelCountModeMax:
      return @"max";
    case ChannelCountModeClampedMax:
      return @"clapmed-max";
    case ChannelCountModeExplicit:
      return @"explicit";
    default:
      @throw [NSException
          exceptionWithName:@"Invalid wave type"
                     reason:@"You have to pick correct channel count mode: ['max', 'clamped-max', 'explicit']"
                   userInfo:nil];
  }

  return nil;
}

@end
