#pragma once

#import <AVFoundation/AVFoundation.h>
#import <Foundation/Foundation.h>

@interface AudioDecoder : NSObject

@property (nonatomic, strong) AVAudioPCMBuffer *buffer;
@property (nonatomic, assign) int sampleRate;

- (instancetype)initWithSampleRate:(int)sampleRate;

- (const AudioBufferList *)decode:(NSString *)pathOrURL;

- (void)cleanup;

@end
