#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface RNAA_AudioBuffer : NSObject

@property (nonatomic, assign) int numberOfChannels;
@property (nonatomic, assign) int length;
@property (nonatomic, assign) int sampleRate;
@property (nonatomic, assign) double duration;
@property (nonatomic, assign) float **channels;

- (instancetype)initWithNumberOfChannels:(int)numberOfChannels length:(int)length sampleRate:(int)sampleRate;

- (float *)getChannelDataForChannel:(int)channel;

- (void)setChannelData:(int)channel data:(float *)data length:(int)length;

- (void)cleanup;

- (RNAA_AudioBuffer *)mixWithOutputNumberOfChannels:(int)outputNumberOfChannels;

@end
