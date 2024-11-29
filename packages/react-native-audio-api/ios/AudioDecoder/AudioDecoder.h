#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface AudioDecoder : NSObject

@property (nonatomic, strong) AVAudioPCMBuffer *buffer;
@property (nonatomic, strong) AVAudioFormat *format;
@property (nonatomic, assign) int sampleRate;

- (instancetype)initWithSampleRate:(int)sampleRate;

- (const AudioBufferList *)decodeWithFilePath:(NSString *)path;

- (void)convertFromFormat:(AVAudioFormat*)format;

- (void)cleanup;

@end

