#import "RNAA_AudioBuffer.h"

@implementation RNAA_AudioBuffer

- (instancetype)initWithNumberOfChannels:(int)numberOfChannels length:(int)length sampleRate:(int)sampleRate
{
  self = [super init];
  if (self) {
    if (!(numberOfChannels == 1 || numberOfChannels == 2)) {
      @throw [NSException exceptionWithName:NSInvalidArgumentException
                                     reason:@"only 1 or 2 channels buffer is allowed"
                                   userInfo:nil];
    }
    _numberOfChannels = numberOfChannels;
    _length = length;
    _sampleRate = sampleRate;
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

- (void)cleanup
{
  _channels = nil;
}

- (RNAA_AudioBuffer *)mixWithOutputNumberOfChannels:(int)outputNumberOfChannels
{
  if (self.numberOfChannels == outputNumberOfChannels) {
    return self;
  }

  switch (self.numberOfChannels) {
    case 1:
      if (outputNumberOfChannels == 2) {
        RNAA_AudioBuffer *outputBuffer = [[RNAA_AudioBuffer alloc] initWithNumberOfChannels:2
                                                                                     length:self.length
                                                                                 sampleRate:self.sampleRate];
        float *channelData = [self getChannelDataForChannel:0];
        [outputBuffer setChannelData:0 data:channelData length:self.length];
        [outputBuffer setChannelData:1 data:channelData length:self.length];
        free(channelData);
        return outputBuffer;
      }
      break;
    case 2:
      if (outputNumberOfChannels == 1) {
        RNAA_AudioBuffer *outputBuffer = [[RNAA_AudioBuffer alloc] initWithNumberOfChannels:1
                                                                                     length:self.length
                                                                                 sampleRate:self.sampleRate];
        float *outputData = (float *)malloc(self.length * sizeof(float));

        for (int i = 0; i < self.length; i++) {
          outputData[i] = (self.channels[0][i].floatValue + self.channels[1][i].floatValue) / 2;
        }

        [outputBuffer setChannelData:0 data:outputData length:self.length];
        free(outputData);
        return outputBuffer;
      }
      break;
  }

  @throw [NSException exceptionWithName:NSInvalidArgumentException
                                 reason:@"Unsupported number of channels"
                               userInfo:nil];
}

@end
