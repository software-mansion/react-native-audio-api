#pragma once

@interface RNAA_AudioBuffer : NSObject

@property (nonatomic, readonly) int sampleRate;
@property (nonatomic, readonly) int length;
@property (nonatomic, readonly) int numberOfChannels;
@property (nonatomic, readonly) double duration;
@property (nonatomic) float **channels;

- (instancetype)initWithSampleRate:(int)sampleRate
                         length:(int)length
               numberOfChannels:(int)numberOfChannels;

- (float **)getChannelDataForChannel:(int)channel;
- (void)setChannelData:(int)channel data:(float **)data length:(int)length;
- (RNAA_AudioBuffer *)copyBuffer;
- (RNAA_AudioBuffer *)mixWithOutputNumberOfChannels:(int)outputNumberOfChannels;
- (void)cleanup;

@end
