#import "RNAA_AudioBuffer.h"

@implementation RNAA_AudioBuffer

- (instancetype)initWithNumberOfChannels:(int)numberOfChannels length:(int)length sampleRate:(int)sampleRate
{
  self = [super init];
  
  if (!self) {
    return  self;
  }
  
  _numberOfChannels = numberOfChannels;
  _length = length;
  _sampleRate = sampleRate;
  _duration = (double)length / sampleRate;
  
  if (!numberOfChannels || numberOfChannels > 2) {
    @throw [NSException exceptionWithName:NSInvalidArgumentException
                                   reason:@"only 1 or 2 channels buffer is allowed"
                                 userInfo:nil];
  }

  // Allocate the channels array
  _channels = malloc(numberOfChannels * sizeof(float *));
  
  for (int i = 0; i < numberOfChannels; i += 1) {
    _channels[i] = malloc(_length * sizeof(float));
    
    for (int j = 0; j < _length; j += 1) {
      _channels[i][j] = 0.0f;
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

  return _channels[channel];
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
  
  memcpy(_channels[channel], data, length * sizeof(float));
}

- (void)cleanup
{
  for (int i = 0; i < _numberOfChannels; i += 1) {
    free(_channels[i]);
  }
  
  free(_channels);
}

- (RNAA_AudioBuffer *)mixWithOutputNumberOfChannels:(int)outputNumberOfChannels
{
  if (self.numberOfChannels == outputNumberOfChannels) {
    return self;
  }
  
  switch (self.numberOfChannels) {
    case 1: {
      RNAA_AudioBuffer *outputBuffer = [[RNAA_AudioBuffer alloc] initWithNumberOfChannels:2
                                                                                   length:self.length
                                                                               sampleRate:self.sampleRate];
      [outputBuffer setChannelData:0 data:self.channels[0] length:self.length];
      [outputBuffer setChannelData:1 data:self.channels[0] length:self.length];
      return outputBuffer;
    }
    case 2: {
      RNAA_AudioBuffer *outputBuffer = [[RNAA_AudioBuffer alloc] initWithNumberOfChannels:1
                                                                                   length:self.length
                                                                               sampleRate:self.sampleRate];
      float *outputData = [outputBuffer getChannelDataForChannel:0];
      
      for (int i = 0; i < self.length; i += 1) {
        outputData[i] = (self.channels[0][i] + self.channels[1][i]) / 2;
      }
      return  outputBuffer;
    }
  }

  @throw [NSException exceptionWithName:NSInvalidArgumentException
                                 reason:@"Unsupported number of channels"
                               userInfo:nil];
}

@end
