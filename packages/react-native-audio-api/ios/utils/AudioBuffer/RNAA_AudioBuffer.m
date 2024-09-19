#import "RNAA_AudioBuffer.h"

@implementation RNAA_AudioBuffer

- (instancetype)initWithSampleRate:(int)sampleRate length:(int)length numberOfChannels:(int)numberOfChannels
{
  self = [super init];
  if (self) {
      if (!(numberOfChannels == 1 || numberOfChannels == 2)) {
        @throw [NSException exceptionWithName:NSInvalidArgumentException
                                       reason:@"only 1 or 2 channels buffer is allowed"
                                     userInfo:nil];
      }
    _sampleRate = sampleRate;
    _length = length;
    _numberOfChannels = numberOfChannels;
    _duration = (double)length / sampleRate;

    // Allocate the channels array
    _channels = [NSMutableArray arrayWithCapacity:numberOfChannels];
    for (int i = 0; i < numberOfChannels; i++) {
      NSMutableArray<NSNumber *> *channelData = [NSMutableArray arrayWithCapacity:length];
      for (int j = 0; j < length; j++) {
        [channelData addObject:@(0.0f)];
      }
      [_channels addObject:channelData];
    }
  }

  return self;
}

- (float *)getChannelDataForChannel:(int)channel
{
  if (channel < 0 || channel >= self.numberOfChannels) {
    @throw [NSException exceptionWithName:NSInvalidArgumentException
                                   reason:@"Channel index out of bounds"
                                 userInfo:nil];
  }

  NSMutableArray<NSNumber *> *channelData = self.channels[channel];
    
    float *data = (float *)malloc(_length * sizeof(float));
  for (int i = 0; i < self.length; i++) {
      data[i] = channelData[i].floatValue;
  }
  return data;
}

- (void)setChannelData:(int)channel data:(float *)data length:(int)length
{
  if (channel < 0 || channel >= self.numberOfChannels) {
    @throw [NSException exceptionWithName:NSInvalidArgumentException
                                   reason:@"Channel index out of bounds"
                                 userInfo:nil];
  }
  if (length != self.length) {
    @throw [NSException exceptionWithName:NSInvalidArgumentException
                                   reason:@"Data length does not match buffer length"
                                 userInfo:nil];
  }

  NSMutableArray<NSNumber *> *channelData = self.channels[channel];
  for (int i = 0; i < length; i++) {
    channelData[i] = @(data[i]);
  }
}

- (void)cleanup {
    _channels = nil;
}

@end
